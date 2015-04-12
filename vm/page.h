#ifndef VM_PAGE_H
#define VM_PAGE_H

#include <hash.h>
#include "filesys/off_t.h"

struct page
{
	void *addr; /* The address of this page */
	bool resident_bit; /* Set 1 if the page is in physical memory, 0 otherwise */
	bool in_swap; /* Bool to tell if the page is currently in swap */
	bool in_filesys; /* Bool to tell if the page is currently in the file system */
	struct hash_elem page_table_elem; /* Hash Table elem for our supplemental page table */
	bool stack_page; /* true if this page is a stack page */

	//This is the meta data to help with demand paging in page_fault
	struct file *file;
	off_t ofs;
	uint32_t read_bytes;
	uint32_t zero_bytes;
	bool writable;
};

void add_page(struct file *file, off_t ofs, uint8_t *vaddr,
    uint32_t read_bytes, uint32_t zero_bytes, bool writable);
struct page *find_page(void *addr);
// void add_page_to_stack(struct frame * f);


#endif /* vm/page.h */
