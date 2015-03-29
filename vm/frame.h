#ifndef VM_FRAME_H
#define VM_FRAME_H

#include <stddef.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include "threads/palloc.h"

static struct frame_num
{
	int num_frames;
};

struct frame
{
	struct page *cur_page; /* A pointer to the page that is currently using this frame */
	
};

//static size_t num_frames;

void *get_frame();

#endif /* vm/frame.h */
