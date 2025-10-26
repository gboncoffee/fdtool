#include "arena.h"
#include "fds.h"

#include <alloca.h>
#include <assert.h>
#include <stdlib.h>

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

DependencyList*
MinimizeLvalues(DependencyList* list, Arena* arena)
{
    (void) list;
    (void) arena;
    assert(0 && "todo");
    return NULL;
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
    //newSet.dependencies = MinimizeLvalues(newSet.dependencies, &arena);
    PrintFds(newSet);

    DropArena(&arena);

    return 0;
}
