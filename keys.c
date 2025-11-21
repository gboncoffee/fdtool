#include "arena.h"
#include "fds.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

typedef struct KeyList
{
    char            elements[27];
    struct KeyList* next;
} KeyList;

KeyList*
NewKey(Arena* arena, const char* elements, KeyList* next)
{
    KeyList* new = Alloc(arena, sizeof(*new));
    strcpy(new->elements, elements);
    new->next = next;
    return new;
}

const TermList*
FindInTermList(const char term, const TermList* const list)
{
    if (list == NULL)
        return NULL;
    if (list->term == term)
        return list;
    return FindInTermList(term, list->next);
}

static int
IsRvalue(char term, const DependencyList* const list)
{
    if (list == NULL)
        return 0;
    if (FindInTermList(term, list->rvalue) != NULL)
        return 1;
    return IsRvalue(term, list->next);
}

static void
GetEssential(
    char* const                 dest,
    const TermList* const       universe,
    const DependencyList* const deps
)
{
    if (universe == NULL)
    {
        dest[0] = '\0';
        return;
    }
    if (!IsRvalue(universe->term, deps))
    {
        dest[0] = universe->term;
        GetEssential(&dest[1], universe->next, deps);
        return;
    }
    GetEssential(dest, universe->next, deps);
}

static void
GetOthers(
    char* const       dest,
    const TermList*   universe,
    const char* const essential
)
{
    if (universe == NULL)
    {
        dest[0] = '\0';
        return;
    }
    if (strchr(essential, universe->term) == NULL)
    {
        dest[0] = universe->term;
        GetOthers(&dest[1], universe->next, essential);
        return;
    }
    GetOthers(dest, universe->next, essential);
}

static void
CopyMasked(char* const dest, const char* const src, unsigned int mask)
{
    int destI = 0;
    int srcI  = 0;
    for (; mask != 0; mask >>= 1)
    {
        if (mask & 1)
        {
            dest[destI] = src[srcI];
            destI += 1;
        }
        srcI += 1;
    }
}

static int
IsMinimal(const char* key, const Depset set, const char* universe)
{
    unsigned int n = strlen(key);
    if (n == 1)
        return 1;

    unsigned int metaMask    = (1 << n) - 1;
    char         subset[27]  = {0};
    char         closure[27] = {0};
    for (unsigned int mask = 1 << (n - 1); mask > 0; mask >>= 1)
    {
        CopyMasked(subset, key, (~mask) & metaMask);
        GetClosure(set.dependencies, subset, closure);
        if (strcmp(universe, closure) == 0)
            return 0;
    }
    return 1;
}

static KeyList*
GetKeys(
    Arena*            arena,
    const char* const essential,
    const char* const others,
    const Depset      set
)
{
    KeyList*     list    = NULL;
    int          n       = strlen(others);
    unsigned int maximum = 1 << n;

    char universe[27];
    CopyTermList(universe, set.universe);
    SortString(universe);

    char key[27] = {0};
    strcpy(key, essential);
    int  ness        = strlen(essential);
    char closure[27] = {0};

    for (unsigned int mask = 0; mask < maximum; mask += 1)
    {
        CopyMasked(&key[ness], others, mask);
        GetClosure(set.dependencies, key, closure);
        if (strcmp(universe, closure) == 0 && IsMinimal(key, set, universe))
            list = NewKey(arena, key, list);
    }

    return list;
}

void
PrintKeys(const KeyList* const list)
{
    if (list == NULL)
        return;
    printf("%s\n", list->elements);
    PrintKeys(list->next);
}

int
Keys(const Depset set)
{
    Arena arena;
    NewArena(&arena, 4096);

    char essential[27] = {0};
    char others[27]    = {0};
    GetEssential(essential, set.universe, set.dependencies);
    GetOthers(others, set.universe, essential);

    KeyList* keys = GetKeys(&arena, essential, others, set);
    PrintKeys(keys);

    DropArena(&arena);
    return 0;
}
