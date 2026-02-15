#include "lua_pool.h"

#include <stdbool.h>
#include <string.h>

#include "lua/lua.h"
#include "lua/lualib.h"
#include "lua/lauxlib.h"

#include "lua_functions.h"
#include "memory.h"

typedef struct BlockHeader {
    size_t size;  // usable size (excluding header)
    bool free;
} BlockHeader;

#define BLOCK_HDR_SIZE (sizeof(BlockHeader))
#define MIN_BLOCK_SIZE 8

static bool lua_heap_initialized = false;
static size_t lua_heap_used = 0;

static void lua_heap_init(void) {
    BlockHeader *first = (BlockHeader *)tinybit_memory->lua_state;
    first->size = TB_MEM_LUA_STATE_SIZE - BLOCK_HDR_SIZE;
    first->free = true;
    lua_heap_initialized = true;
    lua_heap_used = 0;
}

static void *pool_alloc(size_t size) {
    // align to pointer size
    size = (size + sizeof(void*) - 1) & ~(sizeof(void*) - 1);

    uint8_t *pos = tinybit_memory->lua_state;
    while (pos < tinybit_memory->lua_state + TB_MEM_LUA_STATE_SIZE) {
        BlockHeader *hdr = (BlockHeader *)pos;
        if (hdr->free && hdr->size >= size) {
            // split if remaining space is large enough
            if (hdr->size >= size + BLOCK_HDR_SIZE + MIN_BLOCK_SIZE) {
                BlockHeader *next = (BlockHeader *)(pos + BLOCK_HDR_SIZE + size);
                next->size = hdr->size - size - BLOCK_HDR_SIZE;
                next->free = true;
                hdr->size = size;
            }
            hdr->free = false;
            lua_heap_used += hdr->size;
            return pos + BLOCK_HDR_SIZE;
        }
        pos += BLOCK_HDR_SIZE + hdr->size;
    }
    return NULL; // out of memory
}

static void pool_free(void *ptr) {
    if (!ptr) return;

    BlockHeader *hdr = (BlockHeader *)((uint8_t *)ptr - BLOCK_HDR_SIZE);
    hdr->free = true;
    lua_heap_used -= hdr->size;

    // coalesce with next block
    uint8_t *next_pos = (uint8_t *)ptr + hdr->size;
    if (next_pos < tinybit_memory->lua_state + TB_MEM_LUA_STATE_SIZE) {
        BlockHeader *next = (BlockHeader *)next_pos;
        if (next->free) {
            hdr->size += BLOCK_HDR_SIZE + next->size;
        }
    }

    // coalesce with previous block (scan from start)
    uint8_t *pos = tinybit_memory->lua_state;
    while (pos < (uint8_t *)hdr) {
        BlockHeader *prev = (BlockHeader *)pos;
        uint8_t *prev_end = pos + BLOCK_HDR_SIZE + prev->size;
        if (prev_end == (uint8_t *)hdr && prev->free) {
            prev->size += BLOCK_HDR_SIZE + hdr->size;
            break;
        }
        pos = prev_end;
    }
}

static void *l_alloc_pool(void *ud, void *ptr, size_t osize, size_t nsize) {
    (void)ud;

    if (!lua_heap_initialized) {
        lua_heap_init();
    }

    if (nsize == 0) {
        pool_free(ptr);
        return NULL;
    }

    if (ptr == NULL) {
        return pool_alloc(nsize);
    }

    // realloc: allocate new block, copy, free old
    void *new_ptr = pool_alloc(nsize);
    if (new_ptr) {
        memcpy(new_ptr, ptr, osize < nsize ? osize : nsize);
        pool_free(ptr);
    }
    return new_ptr;
}

lua_State* lua_pool_newstate(void) {
    lua_State *L = lua_newstate(l_alloc_pool, NULL);
    if (L) {
        lua_setup(L);
    }
    return L;
}

size_t lua_pool_get_used(void) {
    return lua_heap_used;
}
