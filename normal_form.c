#include "arena.h"
#include "fds.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

extern DependencyList* DupDependencyList(DependencyList* list, Arena* arena);
extern DependencyList* SeparateRvalues(DependencyList* list, Arena* arena);
extern void MinimizeLvalues(const Depset set, DependencyList* list, Arena* arena);

typedef struct KeyList
{
    char            elements[27];
    struct KeyList* next;
} KeyList;

static bool
IsInTermList(char term, const TermList* list)
{
    while (list != NULL)
    {
        if (list->term == term)
            return true;
        list = list->next;
    }
    return false;
}

// verifica se term aparece em algum RHS
static bool
IsRvalue(char term, const DependencyList* list)
{
    while (list != NULL)
    {
        if (IsInTermList(term, list->rvalue))
            return true;
        list = list->next;
    }
    return false;
}

// obtem atributos essenciais (nunca aparecem no RHS)
static void
GetEssential(
    char* dest,
    const TermList* universe,
    const DependencyList* deps
)
{
    int i = 0;
    while (universe != NULL)
    {
        if (!IsRvalue(universe->term, deps))
        {
            dest[i++] = universe->term;
        }
        universe = universe->next;
    }
    dest[i] = '\0';
}

// trata dos outros atributos (não essenciais)
static void
GetOthers(char* dest, const TermList* universe, const char* essential)
{
    int i = 0;
    while (universe != NULL)
    {
        if (strchr(essential, universe->term) == NULL)
        {
            dest[i++] = universe->term;
        }
        universe = universe->next;
    }
    dest[i] = '\0';
}

static void
CopyMasked(char* dest, const char* src, unsigned int mask)
{
    int destI = 0;
    int srcI = 0;
    for (; mask != 0; mask >>= 1)
    {
        if (mask & 1)
        {
            dest[destI++] = src[srcI];
        }
        srcI++;
    }
    dest[destI] = '\0';
}

static bool
IsMinimal(const char* key, const Depset set, const char* universe)
{
    unsigned int n = strlen(key);
    if (n == 1)
        return true;

    unsigned int metaMask = (1 << n) - 1;
    char subset[27] = {0};
    char closure[27] = {0};
    
    for (unsigned int mask = 1 << (n - 1); mask > 0; mask >>= 1)
    {
        CopyMasked(subset, key, (~mask) & metaMask);
        GetClosure(set.dependencies, subset, closure);
        if (strcmp(universe, closure) == 0)
            return false;
    }
    return true;
}

static KeyList*
AddKey(Arena* arena, const char* elements, KeyList* next)
{
    KeyList* new = Alloc(arena, sizeof(*new));
    strcpy(new->elements, elements);
    new->next = next;
    return new;
}

static KeyList*
GetAllKeys(
    Arena* arena,
    const char* essential,
    const char* others,
    const Depset set
)
{
    KeyList* list = NULL;
    int n = strlen(others);
    unsigned int maximum = 1 << n;

    char universe[27];
    CopyTermList(universe, set.universe);
    SortString(universe);

    char key[27] = {0};
    strcpy(key, essential);
    int ness = strlen(essential);
    char closure[27] = {0};

    for (unsigned int mask = 0; mask < maximum; mask++)
    {
        CopyMasked(&key[ness], others, mask);
        GetClosure(set.dependencies, key, closure);
        if (strcmp(universe, closure) == 0 && IsMinimal(key, set, universe))
        {
            list = AddKey(arena, key, list);
        }
    }

    return list;
}

static bool
IsPrime(char attr, const KeyList* keys)
{
    while (keys != NULL)
    {
        if (strchr(keys->elements, attr) != NULL)
            return true;
        keys = keys->next;
    }
    return false;
}

static void
RemoveRedundantDependencies(Depset* set, Arena* arena)
{
    DependencyList* prev = NULL;
    DependencyList* cur = set->dependencies;
    
    while (cur != NULL)
    {
        char lhs[27] = {0};
        char closure[27] = {0};
        CopyTermList(lhs, cur->lvalue);

        // cria lista sem a dependência atual
        DependencyList* others = NULL;
        DependencyList* temp = set->dependencies;
        while (temp != NULL)
        {
            if (temp != cur)
            {
                DependencyList* copy = Alloc(arena, sizeof(*copy));
                copy->lvalue = temp->lvalue;
                copy->rvalue = temp->rvalue;
                copy->next = others;
                others = copy;
            }
            temp = temp->next;
        }

        GetClosure(others, lhs, closure);

        if (strchr(closure, cur->rvalue->term) != NULL)
        {
            // redundante, entao remove
            if (prev == NULL)
            {
                set->dependencies = cur->next;
                cur = set->dependencies;
            }
            else
            {
                prev->next = cur->next;
                cur = prev->next;
            }
        }
        else
        {
            prev = cur;
            cur = cur->next;
        }
    }
}

int
NormalForm(const Depset set)
{
    Arena arena;
    NewArena(&arena, 8192);

    // primeiro ele gera uma cobertura minima
    Depset minCover;
    minCover.universe = set.universe;
    minCover.dependencies = SeparateRvalues(
        DupDependencyList(set.dependencies, &arena), 
        &arena
    );
    MinimizeLvalues(minCover, minCover.dependencies, &arena);
    RemoveRedundantDependencies(&minCover, &arena);

    // segundo, calcula todas as chaves candidatas
    char essential[27] = {0};
    char others[27] = {0};
    GetEssential(essential, set.universe, set.dependencies);
    GetOthers(others, set.universe, essential);
    KeyList* keys = GetAllKeys(&arena, essential, others, set);

    // depois, obtem universo como string
    char universe[27];
    CopyTermList(universe, set.universe);
    SortString(universe);

    // quarto, verifica violacoes
    bool bcnfViolation = false;
    bool threeNFViolation = false;

    // arrays para guardar as violações (var temp)
    typedef struct Violation {
        char lhs[27];
        char rhs;
        bool notSuperkey;
        bool notPrime;
        struct Violation* next;
    } Violation;

    Violation* violations = NULL;

    DependencyList* dep = minCover.dependencies;
    while (dep != NULL)
    {
        char lhs[27] = {0};
        CopyTermList(lhs, dep->lvalue);
        char rhs = dep->rvalue->term;

        // ignora dependências triviais (A E L em L -> A)
        if (strchr(lhs, rhs) == NULL)
        {
            // calcular fecho de L
            char closure[27] = {0};
            GetClosure(minCover.dependencies, lhs, closure);

            // verifica se L é superchave (closure(L) = U)
            bool isSuperkey = (strcmp(closure, universe) == 0);

            if (!isSuperkey)
            {
                bcnfViolation = true;

                bool isPrimeAttr = IsPrime(rhs, keys);

                if (!isPrimeAttr)
                {
                    threeNFViolation = true;
                }

                Violation* v = Alloc(&arena, sizeof(*v));
                strcpy(v->lhs, lhs);
                v->rhs = rhs;
                v->notSuperkey = !isSuperkey;
                v->notPrime = !isPrimeAttr;
                v->next = violations;
                violations = v;
            }
        }

        dep = dep->next;
    }

    if (bcnfViolation)
    {
        printf("BCNF: VIOLATIONS\n");
    }
    else
    {
        printf("BCNF: OK\n");
    }

    if (threeNFViolation)
    {
        printf("3NF: VIOLATIONS\n");
    }
    else
    {
        printf("3NF: OK\n");
    }

    // imprimir detalhes das violações
    Violation* v = violations;
    while (v != NULL)
    {
        if (v->notSuperkey)
        {
            printf("VIOLATION BCNF: %s->%c (%s not superkey)\n", 
                   v->lhs, v->rhs, v->lhs);
        }

        if (v->notSuperkey && v->notPrime)
        {
            printf("VIOLATION 3NF: %s->%c (%c not prime, %s not superkey)\n",
                   v->lhs, v->rhs, v->rhs, v->lhs);
        }

        v = v->next;
    }

    DropArena(&arena);
    return 0;
}
