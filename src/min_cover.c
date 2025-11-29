#include "../includes/arena.h"
#include "../includes/fds.h"

#include <alloca.h>
#include <assert.h>
#include <stdbool.h>
#include <string.h>

TermList*
DupTermList(TermList* list, Arena* arena)
{
    if (list == NULL)
        return NULL;

    TermList* new = Alloc(arena, sizeof(*new));
    new->term     = list->term;
    new->next     = DupTermList(list->next, arena);
    return new;
}

DependencyList*
DupDependencyList(DependencyList* list, Arena* arena)
{
    if (list == NULL)
        return NULL;

    DependencyList* new = Alloc(arena, sizeof(*new));
    new->lvalue         = DupTermList(list->lvalue, arena);
    new->rvalue         = DupTermList(list->rvalue, arena);
    new->next           = DupDependencyList(list->next, arena);
    return new;
}

DependencyList*
SeparateRvalues(DependencyList* list, Arena* arena)
{
    if (list == NULL)
        return NULL;
    if (list->rvalue->next == NULL)
    {
        list->next = SeparateRvalues(list->next, arena);
        return list;
    }

    DependencyList* new = Alloc(arena, sizeof(*new));
    new->lvalue         = DupTermList(list->lvalue, arena);
    new->rvalue         = Alloc(arena, sizeof(*new->rvalue));
    new->rvalue->term   = list->rvalue->term;
    new->rvalue->next   = NULL;
    list->rvalue        = list->rvalue->next;
    new->next           = SeparateRvalues(list, arena);

    return new;
}

int
TermListLength(const TermList* const list)
{
    if (list == NULL)
        return 0;
    return 1 + TermListLength(list->next);
}

void
CopyTermList(char* dest, const TermList* const list)
{
    if (list == NULL)
    {
        dest[0] = '\0';
        return;
    }

    dest[0] = list->term;
    CopyTermList(&dest[1], list->next);
}

static void
CopyTermListExceptNode(
    char*                 dest,
    const TermList* const list,
    const TermList* const skipNode
)
{
    const TermList* cur = list;
    int             i   = 0;
    while (cur != NULL)
    {
        if (cur != skipNode)
        {
            dest[i++] = cur->term;
        }
        cur = cur->next;
    }
    dest[i] = '\0';
}

static void
MinimizeLvalueForDependency(DependencyList* allDeps, DependencyList* dep)
{
    // tenta fazer remoções até que nenhum atributo seja estranho
    bool changed = true;
    while (changed && dep->lvalue != NULL && dep->lvalue->next != NULL)
    {
        changed        = false;
        TermList* prev = NULL;
        TermList* cur  = dep->lvalue;

        while (cur != NULL)
        {
            // constroi a string LHS sem o no atual
            char lhs[27]     = {0};
            char closure[27] = {0};
            CopyTermListExceptNode(lhs, dep->lvalue, cur);

            // se remover atual esvazia LHS, ainda é válido testar
            GetClosure(allDeps, lhs, closure);
            if (strchr(closure, dep->rvalue->term) != NULL)
            {
                // atual é estranho, desvincula-o
                if (prev == NULL)
                {
                    dep->lvalue = cur->next;
                    cur         = dep->lvalue;
                }
                else
                {
                    prev->next = cur->next;
                    cur        = prev->next;
                }
                changed = true;
                // reinicia a varredura a partir da (possivelmente) nova cabeça
                break;
            }
            else
            {
                prev = cur;
                cur  = cur->next;
            }
        }
    }
}

void
MinimizeLvalues(const Depset set, DependencyList* list, Arena* arena)
{
    if (list == NULL)
        return;
    MinimizeLvalueForDependency(set.dependencies, list);
    MinimizeLvalues(set, list->next, arena);
}

static DependencyList*
DupDependencyListExcept(
    DependencyList* list,
    DependencyList* skip,
    Arena*          arena
)
{
    if (list == NULL)
        return NULL;
    if (list == skip)
        return DupDependencyListExcept(list->next, skip, arena);

    DependencyList* new = Alloc(arena, sizeof(*new));
    new->lvalue         = DupTermList(list->lvalue, arena);
    new->rvalue         = DupTermList(list->rvalue, arena);
    new->next           = DupDependencyListExcept(list->next, skip, arena);
    return new;
}

static void
RemoveRedundantDependencies(Depset* set, Arena* arena)
{
    DependencyList* prev = NULL;
    DependencyList* cur  = set->dependencies;
    while (cur != NULL)
    {
        // constroi a string LHS da dependência atual
        char lhs[27]     = {0};
        char closure[27] = {0};
        CopyTermList(lhs, cur->lvalue);

        // duplica conjunto de dependências excluindo a dependência atual
        DependencyList* others =
            DupDependencyListExcept(set->dependencies, cur, arena);
        GetClosure(others, lhs, closure);

        if (strchr(closure, cur->rvalue->term) != NULL)
        {
            // atual é redundante, remove-o da lista original
            if (prev == NULL)
            {
                set->dependencies = cur->next;
                cur               = set->dependencies;
            }
            else
            {
                prev->next = cur->next;
                cur        = prev->next;
            }
            // continua sem avançar prev
        }
        else
        {
            prev = cur;
            cur  = cur->next;
        }
    }
}

int
MinCover(const Depset set)
{
    Arena arena;
    NewArena(&arena, 4096);

    Depset newSet;
    newSet.universe = set.universe;
    newSet.dependencies =
        SeparateRvalues(DupDependencyList(set.dependencies, &arena), &arena);
    MinimizeLvalues(newSet, newSet.dependencies, &arena);
    RemoveRedundantDependencies(&newSet, &arena);
    PrintDependencies(newSet.dependencies);

    DropArena(&arena);

    return 0;
}
