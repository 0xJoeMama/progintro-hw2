#ifndef ALLOCATOR_H
#include <stddef.h>
#include <inttypes.h>

typedef void *Allocator_t;

typedef struct {
  void *(*alloc_fn)(void *alloc, size_t sz);
  void *(*calloc_fn)(void *alloc, size_t item_cnt, size_t item_sz);
  void *(*realloc_fn)(void *alloc, void *old, size_t new_sz);
  void (*dealloc_fn)(void *alloc, void *buf);
} AllocFnTable_t;

typedef struct {
  const AllocFnTable_t *fns;
} Mallocator_t;

typedef struct {
  const AllocFnTable_t *fns;
  uint8_t *buf;
  size_t sz;
  uint8_t *cursor;
  Allocator_t parent;
} ArenaAllocator_t;

Mallocator_t mallocator_get(void);
int arena_init(ArenaAllocator_t *arena, size_t sz, Allocator_t parent);
void arena_deinit(ArenaAllocator_t *arena);

#define alloc_get_fns(allocator) ((AllocFnTable_t *)allocator)

#ifdef ALLOC_IMPL
#include <string.h>
#include <stdlib.h>

void *mallocator_alloc(Allocator_t alloc, size_t sz) {
  (void)alloc;
  return malloc(sz);
}

void *mallocator_calloc(Allocator_t alloc, size_t item_cnt, size_t item_sz) {
  (void)alloc;
  return calloc(item_cnt, item_sz);
}

void *mallocator_realloc(Allocator_t alloc, void *buf, size_t new_sz) {
  (void)alloc;
  return realloc(buf, new_sz);
}

void mallocator_dealloc(Allocator_t alloc, void *buf) {
  (void)alloc;
  free(buf);
}

const AllocFnTable_t mallocator_fns = {
    .alloc_fn = mallocator_alloc,
    .calloc_fn = mallocator_calloc,
    .realloc_fn = mallocator_realloc,
    .dealloc_fn = mallocator_dealloc,
};

Mallocator_t mallocator_get(void) {
  return (Mallocator_t){
      .fns = &mallocator_fns,
  };
}

void *arena_alloc(Allocator_t alloc, size_t sz) {
  if (sz == 0)
    return NULL;

  sz += 8;
  ArenaAllocator_t *arena = (ArenaAllocator_t *)alloc;
  /* check if we are within bounds */
  if (arena->cursor + sz >= arena->buf + arena->sz)
    return NULL;

  uint8_t *res = arena->cursor;
  arena->cursor += sz;

  memset(res, 0, sizeof(size_t));
  *(size_t *)res = sz;

  return res + sizeof(size_t);
}

void *arena_calloc(Allocator_t alloc, size_t item_cnt, size_t item_sz) {
  /* notice that since this is fixed size, we can't easily reallocate.
   * Thus instead, we just allocate a new block and return it, after copying the
   * old data there */
  ArenaAllocator_t *arena = (ArenaAllocator_t *)alloc;
  void *new_block = arena_alloc(arena, item_cnt * item_sz);
  if (!new_block) /* old block is left untouched anyway */
    return NULL;

  /* zero initialize result */
  return memset(new_block, 0, item_sz * item_cnt);
}

void arena_dealloc(Allocator_t alloc, void *buf) {
  (void)alloc;
  (void)buf;
  /* do nothing */
}

void *arena_realloc(Allocator_t alloc, void *buf, size_t new_sz) {
  size_t *old_sz = (size_t *)((uint8_t *)buf - sizeof(size_t));
  if (*old_sz < new_sz) {
    /* notice that currently the old block is leaked. THIS IS INTENTIONAL */
    /* reallocation isn't really intended to happen on fixed size allocator */
    void *res = arena_alloc(alloc, new_sz);
    if (!res)
      return NULL;

    /* copy old elements into new buffer */
    return memcpy(res, buf, *old_sz);
  } else if (*old_sz == new_sz) {
    /* same buffer does the job */
    return buf;
  } else {
    /* the new size is smaller than the old one */
    /* we just modify the stored size */
    *old_sz = new_sz;
    return buf;
  }
}

const AllocFnTable_t arena_fns = {
    .alloc_fn = arena_alloc,
    .calloc_fn = arena_calloc,
    .realloc_fn = arena_realloc,
    .dealloc_fn = arena_dealloc,
};

int arena_init(ArenaAllocator_t *arena, size_t sz, Allocator_t parent) {
  memset(arena, 0, sizeof(ArenaAllocator_t));
  uint8_t *buf = (uint8_t *)alloc_get_fns(parent)->alloc_fn(parent, sz);
  if (!buf)
    return 0;

  arena->buf = buf;
  arena->cursor = buf;
  arena->sz = sz;
  arena->fns = &arena_fns;
  arena->parent = parent;

  return 1;
}

void arena_deinit(ArenaAllocator_t *arena) {
  assert(arena->fns && "attempted to deinitialize an uninitialized allocator");
  alloc_get_fns(arena->parent)->dealloc_fn(arena->parent, arena->buf);
  memset(arena, 0, sizeof(ArenaAllocator_t));
}
#endif // ALLOC_IMPL

#define alloc(allocator, bytes) ((allocator)->fns->alloc_fn(allocator, bytes))

#define calloc(allocator, type, cnt)                                           \
  ((allocator)->fns->calloc_fn(allocator, cnt, sizeof(type)))

#define realloc(allocator, buf, new_sz)                                        \
  ((allocator)->fns->realloc_fn(allocator, buf, new_sz))

#define dealloc(allocator, buf) ((allocator)->fns->dealloc_fn(allocator, buf))

#define ALLOCATOR_H
#endif // ALLOCATOR_H
