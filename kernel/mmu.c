
#include <kernel/types.h>

typedef union {
    struct {
        uint64_t present:1;
        uint64_t writable:1;
        uint64_t user:1;
        uint64_t writethrough:1;
        uint64_t nocache:1;
        uint64_t accessed:1;
        uint64_t _available0:1;
        uint64_t size:1;
        uint64_t global:1;
        uint64_t cow_pending:1;
        uint64_t _available2:2;
        uint64_t address :28;
        uint64_t reserved:12;
        uint64_t _available3:11;
        uint64_t noexecute:1;
    } bits;
    uint64_t raw;
} page_t;
#define PAGE_SIZE 4096
#define _pagemap __attribute__((aligned(PAGE_SIZE))) = {0}
page_t paging_pml4t[512] _pagemap;
page_t paging_pdpt[512] _pagemap;