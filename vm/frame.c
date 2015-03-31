#include "vm/frame.h"
#include "lib/kernel/list.h"
#include "threads/loader.h"
#include "threads/malloc.h"
#include "threads/vaddr.h"


static int *evict_frame();

/* Global Frame table list that contains all the available frames that pages can be mapped to*/
struct frame *frame_table;

void
frame_init(){
	frame_table = (struct frame *)malloc(num_frames * sizeof(struct frame *));
}

//This function returns a pointer to a frame
// # Jacob and Kenneth drove here
void * 
get_frame()
{
	bool found_something = false;
	struct frame *f = NULL;
	int *kva = NULL;         //kva = Kernel Vitual Address
	int i;
	//loop through the frame table
	for(i = 0; i < num_frames; i++)
	{
		f = &frame_table[i];
		//Continue looping until you find an empty page
		if(f->cur_page == NULL)
		{
			found_something = true;
			break;
		}
	}
	//if you didn't find an empty frame, evict something.
	if(!found_something)
	{
		ASSERT(0); //for now panic the kernel if frame table is full
		kva = evict_frame();
	}
	//if you did find an empty frame, allocate a page into that frame
	else
	{
		kva = (int *)palloc_get_page(PAL_USER);
		frame_table[i].cur_page = kva;
	}
	ASSERT(kva != NULL);
	return kva;
} // # End Jacob and Kenneth driving

//returns the kva of the new frame
static int *
evict_frame()
{
	//remove first frame from frame_table
	frame_table[0].cur_page = NULL;
	//shift all elements in frame_table to the left
	int i;
	for(i = 0; i < num_frames - 1; i++)
	{
		struct frame *temp = &frame_table[i];
		temp = &frame_table[i+1];
	}
	//put new frame at the last index
	int *kva = (int *)palloc_get_page(PAL_USER);
	frame_table[num_frames - 1].cur_page = kva;
	return kva;
}
