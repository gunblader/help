#ifndef VM_PAGE_H
#define VM_PAGE_H

#include <hash.h>

struct page
{
	void *addr; /* The address of this page */
	int resident_bit; /* Set 1 if the page is in physical memory, 0 otherwise */
	struct hash_elem page_table_elem; /* Hash Table elem for our supplemental page table */
	// int reference_bit; /* Set if it was recently used */
};

#endif /* vm/page.h */
