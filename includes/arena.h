#ifndef ARENA_H_
#define ARENA_H_

typedef struct Arena
{
    char*         mem;
    unsigned long top;
    unsigned long size;
    struct Arena* next;
} Arena;

void  NewArena(Arena* arena, unsigned long size);
void  DropArena(Arena* arena);
void* Alloc(Arena* arena, unsigned long size);

#endif // ARENA_H_
