#include "vm/frame.h"
#include "lib/kernel/list.h"
#include "threads/loader.h"
#include "threads/malloc.h"
#include "threads/vaddr.h"
#include "vm/swap.h"
#include "threads/thread.h"

//Used to track which frame we need to evict when the table is full for FIFO algorithm
int frame_to_evict;

static int *evict_frame();

/*
	Global Frame table list that contains all the available frames that pages
 	can be mapped to
*/
struct frame *frame_table;

void
frame_init(){
	frame_table = (struct frame *)malloc(num_frames * sizeof(struct frame *));
	frame_to_evict = 0;
}

//This function returns a pointer to a frame
// # Jacob and Kenneth drove here
struct frame * 
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
	}
	ASSERT(kva != NULL);
	
	// #paul drove here
	f->kva = kva;
	return f;
	// #driving ends

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




	// #Paul and Adam drove here
	
	//move old page into swap space
	struct page *oldpage = frame_table[frame_to_evict].cur_page;

	swap_page((void *)oldpage->addr);

	//update bool that tells us where this page is
	frame_table[frame_to_evict].cur_page->in_swap = true;

	//need to get rid of page directory entry for this frame
	pagedir_clear_page(thread_current()->pagedir, oldpage->addr);

	//free this frame
	frame_table[frame_to_evict].kva = NULL;

	//allocate a new page to put in the frame
	int *kva = (int *)palloc_get_page(PAL_USER);

	ASSERT(kva != NULL);

	//add the page to this frame
	frame_table[frame_to_evict++].kva = kva;

	if(frame_to_evict >= num_frames)
		frame_to_evict = 0;

	return kva;
	// #End of Paul and Adam driving
}


