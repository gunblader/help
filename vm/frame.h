#ifndef VM_FRAME_H
#define VM_FRAME_H

#include <stddef.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include "threads/palloc.h"
#include "lib/kernel/list.h"
#include "vm/page.h"

// static struct frame_num
// {
// 	int num_frames;
// };

struct frame
{
	// struct page *cur_page; /* A pointer to the page that is currently using this frame */
	void *kva;
	struct list_elem frame_table_elem;
};


void frame_init();
void *get_frame();

size_t num_frames;
#endif /* vm/frame.h */
