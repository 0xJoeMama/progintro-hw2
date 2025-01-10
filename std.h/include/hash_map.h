#ifndef HM_H
#include <assert.h>
#include <inttypes.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define FUDGE 2

#define hm_declare_type(key, value) HashMap_##key##_##value
#define hm_declare_type_long(key, value) HashMap_##key##_##value##_t
#define HashMap(key, value) hm_declare_type(key, value)
#define HashMap_t(key, value) hm_declare_type_long(key, value)
#define KVPair(key, value) KVPair_##key##_##value
#define KVPair_t(key, value) KVPair_##key##_##value##_t
#define hm_function(ret_type, name, key, value, ...)                           \
  ret_type name##_##key##_##value(__VA_ARGS__)

#define hm_function_call(name, key, value) name##_##key##_##value

#define HM_DECLARE(key, value)                                                 \
  typedef struct KVPair(key, value) KVPair_t(key, value);                      \
  struct HashMap(key, value) {                                                 \
    KVPair_t(key, value) * buckets;                                            \
    uint64_t (*hash)(key *);                                                   \
    int (*eq)(key *, key *);                                                   \
    size_t cap;                                                                \
    size_t len;                                                                \
  };                                                                           \
                                                                               \
  hm_function(int, hm_init, key, value, struct HashMap(key, value) * hm,       \
              size_t initial_cap, uint64_t (*hash)(key *),                     \
              int (*eq)(key *, key *));                                        \
  hm_function(int, hm_put, key, value, struct HashMap(key, value) * hm, key k, \
              value v);                                                        \
  hm_function(value *, hm_get, key, value, struct HashMap(key, value) * hm,    \
              key * k);                                                        \
  hm_function(void, hm_deinit, key, value, struct HashMap(key, value) * hm,    \
              void (*destroy)(KVPair_t(key, value)));                          \
  hm_function(int, hm_grow, key, value, struct HashMap(key, value) * hm);      \
  hm_function(int, hm_remove, key, value, struct HashMap(key, value) * hm,     \
              key * k, value * v);                                             \
                                                                               \
  hm_function(KVPair_t(key, value) *, hm_get_entry_raw, key, value,            \
              struct HashMap(key, value) * hm, key * k, uint64_t hash);        \
  typedef struct HashMap(key, value) HashMap_t(key, value);

#define HM_IMPL(key, value)                                                    \
  struct KVPair(key, value) {                                                  \
    key k;                                                                     \
    value v;                                                                   \
    uint64_t hash;                                                             \
    int occupied;                                                              \
  };                                                                           \
                                                                               \
  hm_function(int, hm_init, key, value, struct HashMap(key, value) * hm,       \
              size_t initial_cap, uint64_t (*hash)(key *),                     \
              int (*eq)(key *, key *)) {                                       \
    hm->buckets = (KVPair_t(key, value) *)calloc(                              \
        initial_cap, sizeof(KVPair_t(key, value)));                            \
                                                                               \
    if (!hm->buckets)                                                          \
      return 0;                                                                \
                                                                               \
    hm->cap = initial_cap;                                                     \
    hm->len = 0;                                                               \
    hm->hash = hash;                                                           \
    hm->eq = eq;                                                               \
    return 1;                                                                  \
  }                                                                            \
                                                                               \
  hm_function(int, hm_put, key, value, struct HashMap(key, value) * hm, key k, \
              value v);                                                        \
                                                                               \
  hm_function(KVPair_t(key, value) *, hm_get_entry_raw, key, value,            \
              struct HashMap(key, value) * hm, key * k, uint64_t hash) {       \
    uint64_t reduced_hash = hash % hm->cap;                                    \
    uint64_t pos = reduced_hash;                                               \
    do {                                                                       \
      KVPair_t(key, value) *curr_entry = hm->buckets + pos;                    \
                                                                               \
      if (!curr_entry->occupied || hm->eq(&curr_entry->k, k))                  \
        return curr_entry;                                                     \
                                                                               \
      pos = pos + 1 != hm->cap ? pos + 1 : 0;                                  \
    } while (pos != reduced_hash);                                             \
                                                                               \
    return NULL;                                                               \
  }                                                                            \
                                                                               \
  hm_function(int, hm_grow, key, value, struct HashMap(key, value) * hm) {     \
    KVPair_t(key, value) *new_buckets =                                        \
        calloc(hm->cap * FUDGE, sizeof(KVPair_t(key, value)));                 \
    if (!new_buckets)                                                          \
      return 0;                                                                \
                                                                               \
    struct HashMap(key, value) new_map;                                        \
    if (!hm_function_call(hm_init, key, value)(&new_map, hm->cap * FUDGE,      \
                                               hm->hash, hm->eq))              \
      return 0;                                                                \
                                                                               \
    for (KVPair_t(key, value) *bucket = hm->buckets;                           \
         bucket < hm->buckets + hm->cap; bucket++) {                           \
      if (bucket->occupied) {                                                  \
        uint64_t hash = bucket->hash;                                          \
        KVPair_t(key, value) *new_loc = hm_function_call(                      \
            hm_get_entry_raw, key, value)(&new_map, &bucket->k, hash);         \
        assert(new_loc != NULL && "how did we get here?");                     \
        memcpy(new_loc, bucket, sizeof(KVPair_t(key, value)));                 \
      }                                                                        \
    }                                                                          \
                                                                               \
    new_map.len = hm->len;                                                     \
                                                                               \
    free(hm->buckets);                                                         \
    memcpy(hm, &new_map, sizeof(struct HashMap(key, value)));                  \
                                                                               \
    return 1;                                                                  \
  }                                                                            \
                                                                               \
  /* TODO: make this return the old value in case of override */               \
  hm_function(int, hm_put, key, value, struct HashMap(key, value) * hm, key k, \
              value v) {                                                       \
    uint64_t hash = hm->hash(&k);                                              \
    KVPair_t(key, value) *target_loc =                                         \
        hm_function_call(hm_get_entry_raw, key, value)(hm, &k, hash);          \
    if (target_loc) {                                                          \
      target_loc->occupied = 1;                                                \
      target_loc->k = k;                                                       \
      target_loc->v = v;                                                       \
      target_loc->hash = hash;                                                 \
      hm->len++;                                                               \
      return 1;                                                                \
    }                                                                          \
                                                                               \
    /* this means we need to resize the map */                                 \
    if (!hm_function_call(hm_grow, key, value)(hm))                            \
      /* if resizing fails it means we cannot insert the key */                \
      return 0;                                                                \
                                                                               \
    /* call ourselves with the resized map */                                  \
    return hm_function_call(hm_put, key, value)(hm, k, v);                     \
  }                                                                            \
                                                                               \
  hm_function(int, hm_remove, key, value, struct HashMap(key, value) * hm,     \
              key * k, value * v) {                                            \
    uint64_t hash = hm->hash(k);                                               \
    KVPair_t(key, value) *entry =                                              \
        hm_function_call(hm_get_entry_raw, key, value)(hm, k, hash);           \
                                                                               \
    if (!entry || !entry->occupied)                                            \
      return 0;                                                                \
                                                                               \
    memcpy(v, &entry->v, sizeof(value));                                       \
    entry->occupied = 0;                                                       \
    hm->len--;                                                                 \
                                                                               \
    return 1;                                                                  \
  }                                                                            \
                                                                               \
  hm_function(value *, hm_get, key, value, struct HashMap(key, value) * hm,    \
              key * k) {                                                       \
    uint64_t hash = hm->hash(k);                                               \
                                                                               \
    KVPair_t(key, value) *target_loc =                                         \
        hm_function_call(hm_get_entry_raw, key, value)(hm, k, hash);           \
    if (!target_loc || !target_loc->occupied)                                  \
      return NULL;                                                             \
                                                                               \
    return &target_loc->v;                                                     \
  }                                                                            \
                                                                               \
  hm_function(void, hm_deinit, key, value, struct HashMap(key, value) * hm,    \
              void (*destroy)(KVPair_t(key, value))) {                         \
    if (destroy) {                                                             \
      for (KVPair_t(key, value) *bucket = hm->buckets;                         \
           bucket < hm->buckets + hm->cap; bucket++)                           \
        if (bucket->occupied)                                                  \
          destroy(*bucket);                                                    \
    }                                                                          \
                                                                               \
    free(hm->buckets);                                                         \
    hm->len = 0;                                                               \
    hm->cap = 0;                                                               \
  }

#define HM_DECLARE_IMPL(key, value)                                            \
  HM_DECLARE(key, value)                                                       \
  HM_IMPL(key, value)

#define hm_init(key, value) hm_function_call(hm_init, key, value)
#define hm_put(key, value) hm_function_call(hm_put, key, value)
#define hm_get(key, value) hm_function_call(hm_get, key, value)
#define hm_remove(key, value) hm_function_call(hm_remove, key, value)
#define hm_deinit(key, value) hm_function_call(hm_deinit, key, value)

#define HM_H
#endif
