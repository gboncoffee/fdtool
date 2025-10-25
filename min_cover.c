#include "fds.h"

#include <assert.h>
#include <stdlib.h>

TermList*
DupTermList(TermList* list)
{
    if (list == NULL)
        return NULL;

    TermList* new = malloc(sizeof(*new));
    new->term     = list->term;
    new->next     = DupTermList(list->next);
    return new;
}

DependencyList*
SeparateRvalues(DependencyList* list)
{
    if (list == NULL)
        return NULL;

    if (list->rvalue->next == NULL)
    {
        list->next = SeparateRvalues(list->next);
        return list;
    }

    DependencyList* this = malloc(sizeof(*this));
    this->rvalue         = list->rvalue;
    list->rvalue         = list->rvalue->next;
    this->rvalue->next   = NULL;
    this->lvalue         = DupTermList(this->lvalue);
    this->next           = SeparateRvalues(list);

    return this;
}

int
MinCover(const Depset set)
{
    DependencyList* newList = SeparateRvalues(set.dependencies);
    Depset          newSet;
    newSet.universe     = set.universe;
    newSet.dependencies = newList;
    PrintFds(newSet);

    return 0;
}
