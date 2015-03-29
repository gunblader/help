#include <stdio.h>
#include <stdint.h>
#include "vm/frame.h"
#include "lib/kernel/list.h"
#include "threads/palloc.h"
#include "threads/palloc.c"


// #define NUM_FRAMES num_frames

/* Global Frame table list that contains all the available frames that pages can be mapped to*/
struct frame frame_table[frame_size];


//This function returns a pointer to a frame
// # Jacob and Kenneth drove here
void * 
get_frame()
{
	bool found_something = false;
	struct frame *f = NULL;
	int *kva = NULL;         //kva = Kernel Vitual Address
	int i;
	for(i = 0; i < frame_size; i++)
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
		kva = evict_frame();
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
	for(i = 0; i < frame_size - 1; i++)
	{
		frame_table[i] = frame_table[i+1];
	}
	//put new frame at the last index
	int *kva = (int *)palloc_get_page(PAL_USER);
	frame_table[frame_size - 1] = kva;
	return kva;
}
