#ifndef VM_PAGE_H
#define VM_PAGE_H

#include <hash.h>
#include <stdint.h>
#include "filesys/off_t.h"

struct page
{
	void *addr; /* The address of this page */
	int resident_bit; /* Set 1 if the page is in physical memory, 0 otherwise */
	struct hash_elem page_table_elem; /* Hash Table elem for our supplemental page table */
	// int reference_bit; /* Set if it was recently used */

	/* Meta Data */
	struct thread *cur_thread;
	struct file *file;
	off_t ofs;
	uint8_t *upage;
	uint32_t read_bytes;
	uint32_t zero_bytes;
	bool writable;
	// size_t page_read_bytes;
	// size_t page_zero_bytes;
	bool success;

};

#endif /* vm/page.h */
