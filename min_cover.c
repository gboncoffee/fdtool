#include "arena.h"
#include "fds.h"

#include <alloca.h>
#include <assert.h>
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

TermList*
MinimizeLvaluesInList(DependencyList* list, TermList* lvalue)
{
    if (lvalue->next == NULL)
        return lvalue;

    char* terms = alloca(sizeof(*terms) * (TermListLength(lvalue->next) + 1));
    CopyTermList(terms, lvalue->next);
    char closure[27];

    GetClosure(list, terms, closure);
    if (strchr(closure, list->rvalue->term) != NULL)
        return MinimizeLvaluesInList(list, lvalue->next);

    lvalue->next = MinimizeLvaluesInList(list, lvalue->next);
    return lvalue;
}

void
MinimizeLvalues(const Depset set, DependencyList* list, Arena* arena)
{
    if (list == NULL)
        return;
    if (list->lvalue->next == NULL)
    {
        MinimizeLvalues(set, list->next, arena);
        return;
    }

    TermList* lvalues = MinimizeLvaluesInList(list, list->lvalue);
    list->lvalue      = lvalues;
    MinimizeLvalues(set, list->next, arena);
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
    PrintDependencies(newSet.dependencies);

    DropArena(&arena);

    return 0;
}
