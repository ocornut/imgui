/***********************************************************************************
 * Author: Sepehr Taghdisian (sep.tagh@gmail.com)
 */

#ifndef IMMEM_H
#define IMMEM_H

// Note: This file is meant to included only inside imgui.cpp

#ifndef IMGUI_INTERNAL_USE
  #error "This file is intented for internal use only (as include)"
#endif

/* Linked-List ************************************************************************************/
struct LinkedList
{
    LinkedList *next;
    LinkedList *prev;
    void *data;

    LinkedList() : next(NULL), prev(NULL)  {}
};


static void list_add(struct LinkedList** plist, struct LinkedList* item, void* data)
{
    item->next = (*plist);
    item->prev = NULL;
    if (*plist != NULL)
        (*plist)->prev = item;
    *plist = item;
    item->data = data;
}

static void list_addlast(struct LinkedList** plist, struct LinkedList* item, void* data)
{
    if (*plist != NULL)     {
        struct LinkedList* last = *plist;
        while (last->next != NULL)    last = last->next;
        last->next = item;
        item->prev = last;
        item->next = NULL;
    }    else    {
        *plist = item;
        item->prev = item->next = NULL;
    }

    item->data = data;
}

static void list_remove(struct LinkedList** plist, struct LinkedList* item)
{
    if (item->next != NULL)     item->next->prev = item->prev;
    if (item->prev != NULL)     item->prev->next = item->next;
    if (*plist == item)         *plist = item->next;
    item->next = item->prev = NULL;
}

/* PoolAlloc **************************************************************************************/
template <typename T>
class PoolAlloc
{
private:
    LinkedList *m_blocks;     /* first node of m_blocks */
    int m_block_cnt;
    int m_items_max;  /* maximum number of items allowed (per block) */
    ImGui_MallocCallback m_malloc;
    ImGui_FreeCallback m_free;

private:
    struct Block
    {
        LinkedList node; /* linked-list node */
        unsigned char *buffer; /* memory buffer that holds all objects */
        void **ptrs; /* pointer references to the buffer */
        int iter; /* iterator for current buffer position */
    };

private:
    Block* create_block(int block_size)
    {
        // Allocate in one call
        size_t total_sz =
            sizeof(Block) +
            sizeof(T)*block_size +
            sizeof(void*)*block_size;
        unsigned char *buff = (unsigned char*)m_malloc(total_sz);
        if (buff == NULL)
            return NULL;
        memset(buff, 0x00, total_sz);

        Block *block = (Block*)buff;
        buff += sizeof(Block);
        block->buffer = buff;
        buff += sizeof(T)*block_size;
        block->ptrs = (void**)buff;

        // Assign pointer refs
        for (int i = 0; i < block_size; i++)
            block->ptrs[block_size-i-1] = block->buffer + i*sizeof(T);
        block->iter = block_size;

        /* add to linked-list of the pool */
        list_addlast(&m_blocks, &block->node, block);
        m_block_cnt++;
        return block;
    }

    void destroy_block(Block *block)
    {
        list_remove(&m_blocks, &block->node);
        m_free(block);
        m_block_cnt--;
    }

public:
    PoolAlloc()
    {
        m_blocks = NULL;
        m_block_cnt = 0;
        m_items_max = 0;
        m_malloc = NULL;
        m_free = NULL;
    }

    bool create(int block_sz, ImGui_MallocCallback malloc_fn, ImGui_FreeCallback free_fn)
    {
        m_items_max = block_sz;
        m_malloc = malloc_fn;
        m_free = free_fn;

        // First block
        Block *block = create_block(block_sz);
        if (block == NULL)  {
            destroy();
            return false;
        }

        return true;
    }

    void destroy()
    {
        LinkedList* node = m_blocks;
        while (node != NULL)    {
            LinkedList* next = node->next;
            destroy_block((Block*)node->data);
            node = next;
        }
    }

    T* alloc()
    {
        LinkedList* node = m_blocks;

        while (node != NULL)   {
            Block *block = (Block*)node->data;
            if (block->iter > 0)
                return (T*)block->ptrs[--block->iter];

            node = node->next;
        }

        /* couldn't find a free block, create a new one */
        Block *block = create_block(m_items_max);
        if (block == NULL)
            return NULL;

        return (T*)block->ptrs[--block->iter];
    }

    void free(T *ptr)
    {
        // find the block that pointer belongs to, and free the pointer from that block
        LinkedList *node = m_blocks;
        int buffer_sz = m_items_max*sizeof(T);
        unsigned char *u8ptr = (unsigned char*)ptr;

        while (node != NULL)   {
            Block *block = (Block*)node->data;
            if (u8ptr >= block->buffer && u8ptr < (block->buffer + buffer_sz))  {
                IM_ASSERT(block->iter != m_items_max);
                block->ptrs[block->iter++] = ptr;
                return;
            }
            node = node->next;
        }

        // Memory block does not belong to the pool?!
        IM_ASSERT(0);
    }

    void clear()
    {
        int block_size = m_items_max;
        LinkedList* node = m_blocks;
        while (node != NULL)    {
            Block *block = (Block*)node->data;

            /* only re-assign pointer references to buffer */
            for (int i = 0; i < block_size; i++)
                block->ptrs[block_size-i-1] = block->buffer + i*sizeof(T);
            block->iter = block_size;

            node = node->next;
        }
    }
};

#endif // IMMEM_H
