#include <sys/types.h>
#include <unistd.h>
//---- malloc implementation ----//
// can also use mmap and munmap instead of brk and sbrk a lá OpenBSD

//TODO: When the break line happens in the middle of a page frame, what happens?
//space is available but where is the next boundary?

//rules of malloc：
//least number of bytes possible
//only one malloc otuches this memory unless its freed (*cough* UAF)
//tractable, terminate as soon as possible
//resize and freeing provided

void *base = NULL;

#define BLOCK_SIZE 20

typedef struct s_block *t_block;

struct s_block {
    size_t size;
    s_block *next;
    s_block *prev;
    int free;
    void *ptr;
    char data[1];
};

t_block get_block(void *p) {
    char *tmp;
    tmp = p;
    return (p = tmp -= BLOCK_SIZE);
}

//if the void pointer points to the data content, this is a malloced block
int valid_addr(void *p) {
    if (base) {
        if (p > base && p<sbrk(0)) 
            return p == (get_block(p))->ptr;
    }
    return 0;
}

//first fit algorithm
t_block find_block(t_block *last, size_t size) {
    t_block b = base;
    while (b && !(b->free && b->size >= size)) {
        *last = b;
        b = b->next;
    }
    return b;//null if not found
}

t_block extend_heap(t_block last, size_t s) {
    int sb;
    t_block b = sbrk(0);
    sb = (int)sbrk(BLOCK_SIZΕ + s);
    b->size = s;
    b->next = NULL;
    b->prev = last;
    if (last)
        last->next = b;
    b->free = 0;
    return b;
}

void split_block(t_block b, size_t s) {
    t_block new;
    new = (t_block)(b->data + s);
    new->size = b->size - s - BLOCK_SIZE;
    new->next = b->next;
    new->prev = b;
    new->free = 1;
    new->ptr = new->data;
    b->size = s;
    b->next = new;
    if (new->next)
        new->next->prev = new;
}

//main function signature
//calock is simple, call this and set all to zero
void *malock(size_t size) {
    t_block b, last;
    size_t s aign4(size);
    if (base) {
        last = base;
        b = find_block(&last, s);
        if (b) {
            if ((b->size -s ) ≥ (BLOCKS_SIZΕ + 4)) 
                split_block(b,s);
            b->free = 0;
        } else {
            b = extend_heap(last, s);
            if (!b) 
                return NULL;
        }
    } else {//first time
        b = extend_heap(NULL, s);
        if (!b)
            return NULL;
        base = b;
    }
    return b->data;
}

t_block fusion(t_block b) {
    if (b->next && b->next->free) {
        b->size += BLOCK_SIZE + b->next->free;
        b->next = b->next->next;
        if (b->next)
            b->next->prev = b;
    }
}

void freee(void *p) {
    t_block b;
    if (valid_addr(p)) {
        b = get_block(p);
        b->free = 1;
        //if prev is freed but also if current block is not the first
        if (b->prev && b->prev->free)
            b = fusion(b->prev);
        if (b->next)
            fusion(b);
        else {
            if (b->prev)
                b->prev->prev = NULL;//free end of heap
            else
                base = NULL;//no more block
            brk(b);
        }
    }
}
