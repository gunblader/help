#include <stdio.h>
#include <stdint.h>
#include "vm/frame.h"
#include "lib/kernel/list.h"
#include "threads/palloc.h"

#define NUM_FRAMES = num_frames;

void *get_frame();
int *evict_frame();
/* Global Frame table list that contains all the available frames that pages can be mapped to*/
struct frame *frame_table[NUM_FRAMES];

//This function returns a pointer to a frame
// # Jacob and Kenneth drove here
void * 
get_frame()
{
	bool found_something = false;
	struct frame *f = NULL;
	int *kva = NULL;
	int i;
	for(i = 0; i < NUM_FRAMES; i++)
	{
		f = frame_table[i];
		if(f->cur_page == NULL)
		{
			found_something = true;
			break;
		}
	}
	if(!found_something)
	{
		evict_frame();
	}
	else
	{
		//somehow add that frame (kva) to the frame_table
		kva = (int *)palloc_get_page(PAL_USER);
		frame_table[i] = kva;
	}
	ASSERT(kva != NULL);
	return kva;
} // # End Jacob and Kenneth driving

//returns the kva of the new frame
int *
evict_frame()
{
	//remove first frame from frame_table
	frame_table[0] = NULL;
	//shift all elements in frame_table to the left
	int i;
	for(i = 0; i < NUM_FRAMES - 1; i++)
	{
		frame_table[i] = frame_table[i+1];
	}
	//put new frame at the last index
	int *kva = (int *)palloc_get_page(PAL_USER);
	frame_table[NUM_FRAMES - 1] = kva;
	return kva;
}
