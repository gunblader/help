#include "vm/frame.h"
#include "lib/kernel/list.h"
#include "threads/loader.h"
#include "threads/malloc.h"
#include "threads/vaddr.h"
#include "vm/swap.h"
#include "threads/thread.h"
#include "threads/loader.h"

// Used to track which frame we need to evict when the table is full for FIFO algorithm
int frame_to_evict;

// Number of pages that were returned from the user pool
int num_pages;
bool num_pages_set;

static int *evict_frame();

/*
	Global Frame table list that contains all the available frames that pages
 	can be mapped to
*/
struct frame *frame_table;

void
frame_init(){
	// printf("\n\nnum_frames = %i\n", num_frames);
	frame_table = (struct frame *)malloc(num_frames * sizeof(struct frame));
	frame_to_evict = 1;
	// num_pages = 0;
	// num_pages_set = false;
}

void
frame_table_print(){
	int i;
	for(i = 1; i < num_frames; i++){
		struct frame *f = &frame_table[i];
		printf("**********frame addr #%d: 0x%x, page addr: 0x%x\n", i, f->kva,
		 f->cur_page->addr);
	}
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
		printf("Frame Table full\n");
		
		//frame_table_print();
		// ASSERT(0); //for now panic the kernel if frame table is full
		kva = evict_frame();
		// printf("Evicted Frame kva: 0x%x\n", kva);
		// num_pages_set = true;

	}
	//if you did find an empty frame, allocate a page into that frame
	else
	{
		kva = (int *)palloc_get_page(PAL_USER);
	       
		// printf("********************%s\n", thread_current()->name);
		// struct page *temp = (struct page *)malloc(sizeof(struct page));
		// // temp->addr = kva;
		// temp->in_swap = false;
		// temp->stack_page = false;

		// f->cur_page = temp;

		// if(!num_pages_set)
		// 	num_pages++;
	}
	ASSERT(kva != NULL);
	
	// #paul drove here
	// pagedir_set_accessed(thread_current()->pagedir, f->cur_page->addr, 0);
	f->kva = kva;
	// printf("frame kva: 0x%x\n", f->kva);
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




	// // #Paul and Adam drove here
	
	// //move old page into swap space
	// struct page *oldpage = frame_table[frame_to_evict].cur_page;
	// //update bool that tells us where this page is
	// oldpage->in_swap = true;
	// frame_table[frame_to_evict].kva = NULL;

	// swap_page((void *)oldpage->addr);

	// //need to get rid of page directory entry for this frame
	// pagedir_clear_page(thread_current()->pagedir, oldpage->addr);
	// //free this frame
	// palloc_free_page(oldpage->addr);

	// //allocate a new page to put in the frame
	// int *kva = (int *)palloc_get_page(PAL_USER);

	// ASSERT(kva != NULL);

	// //add the page to this frame
	// frame_table[frame_to_evict++].kva = kva;

	// if(frame_to_evict >= num_frames)
	// 	frame_to_evict = 0;





	// DONT THROW OUT DIRTY PAGES WHEN USING CLOCK*************
	struct thread *cur_thread = thread_current();
	int i = frame_to_evict;
	bool not_found = true;
	struct frame *f = NULL;
	int *kva = NULL;
	// printf("Evicting Frame\n");
	while(not_found)
	{
		// printf("In Evict Frame While Loop\n");
		f = &frame_table[i];
		// printf("Frame f's cur_page: 0x%x\n", f->cur_page->addr);
		//this is the page we want to replace
		// bool is_accessed = pagedir_is_accessed(cur_thread->pagedir, f->cur_page->addr);
		// bool is_dirty = pagedir_is_dirty(cur_thread->pagedir, f->cur_page->addr);
		if( !pagedir_is_accessed(cur_thread->pagedir, f->cur_page->addr) )
		{
			// printf("In Evict Frame conditional\n");
			not_found = false;

			/***perform eviction***/
			printf("Evicting upage into swap from frame table: 0x%x\n", f->cur_page->addr);
			//move old page into swap space
			struct page *oldpage = f->cur_page;
			//update bool that tells us where this page is
			oldpage->in_swap = true;
			// f->kva = NULL;

			//do we want the oldpage to store where it is in swap?
			//void *swap_addr = swap_page((void *)oldpage->addr);
			swap_page((void *)oldpage->addr, f->kva);

			//need to get rid of page directory entry for this frame
			pagedir_clear_page(thread_current()->pagedir, oldpage->addr);
			
			//free this frame
			// printf("Oldpage addr: 0x%x\n", oldpage->addr);
			palloc_free_page(f->kva);

			//allocate a new page to put in the frame
			kva = (int *)palloc_get_page(PAL_USER);

			ASSERT(kva != NULL);

			//add the page to this frame
			frame_table[frame_to_evict++].kva = kva;

		}
		//We set reference bit to 0
		else
		{
			// printf("IN Evict Frame else statement\n");
			pagedir_set_accessed(cur_thread->pagedir, f->cur_page->addr, 0);
		}
		// printf("BEFORE i %i\n", i);
		i = (i == num_frames - 1) ? 1 : i + 1;
		// printf("AFTER i %i\n", i);
	}

	return kva;
	// #End of Paul and Adam driving
}

//removes the page with "uaddr" address from the frame table
struct frame *
lookup_frame(void *uaddr)
{
	struct frame *f = NULL;
	int i;
	for(i = 0; i < num_frames; i++){
		f = &frame_table[i];
		if(f->cur_page != NULL)
		{
			if(f->cur_page->addr == uaddr)
			{
				break;
			}
		}
	}
	return f;
}
