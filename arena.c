#include "arena.h"

#include <assert.h>
#include <stdlib.h>

void
NewArena(Arena* arena, unsigned long size)
{
    arena->mem  = malloc(size);
    arena->size = 4096;
    arena->top  = 0;
    arena->next = NULL;
}

void
DropArena(Arena* arena)
{
    Arena* next = arena->next;
    free(arena->mem);
    if (next != NULL)
    {
        DropArena(arena->next);
        free(next);
    }
}

void*
Alloc(Arena* arena, unsigned long size)
{
    if (arena->top + size < arena->size)
    {
        void* ptr = &arena->mem[arena->top];
        arena->top += size;
        return ptr;
    }

    if (arena->next == NULL)
    {
        arena->next = malloc(sizeof(*arena->next));
        NewArena(arena->next, size > arena->size ? size : arena->size);
    }

    return Alloc(arena->next, size);
}
