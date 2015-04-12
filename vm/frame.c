#include "vm/frame.h"
#include "lib/kernel/list.h"
#include "threads/loader.h"
#include "threads/malloc.h"
#include "threads/vaddr.h"

int frame_to_evict;

static int *evict_frame();

/* Global Frame table list that contains all the available frames that pages can be mapped to*/
struct frame *frame_table;

void
frame_init(){
	frame_table = (struct frame *)malloc(num_frames * sizeof(struct frame *));
	frame_to_evict = 0;
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
		if(f->kva == NULL)
		{
			found_something = true;
			break;
		}
	}
	//if you didn't find an empty frame, evict something.
	if(!found_something)
	{
		// ASSERT(0); //for now panic the kernel if frame table is full
		kva = evict_frame();
	}
	//if you did find an empty frame, allocate a page into that frame
	else
	{
		kva = (int *)palloc_get_page(PAL_USER);
		f->kva = kva;
	}
	ASSERT(kva != NULL);
	return kva;
} // # End Jacob and Kenneth driving

static int *
evict_frame()
{
	// //THIS NEEDS TO BE CHANGED LATER WHEN WE IMPLEMENT SWAP
	// //remove first frame from frame_table
	// frame_table[0].kva = NULL;
	// //shift all elements in frame_table to the left
	// int i;
	// for(i = 0; i < num_frames - 1; i++)
	// {
	// 	struct frame *temp = &frame_table[i];
	// 	temp = &frame_table[i+1];
	// }
	// //put new frame at the last index
	// int *kva = (int *)palloc_get_page(PAL_USER);
	// frame_table[num_frames - 1].kva = kva;
	// return kva;

	frame_table[frame_to_evict].kva = NULL;
	int *kva = &frame_table[frame_to_evict];

	ASSERT(kva != NULL);
	// frame_table[frame_to_evict++].kva = kva;

	if(frame_to_evict >= num_frames)
		frame_to_evict = 0;

	return kva;

}
