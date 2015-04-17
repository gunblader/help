#ifdef VM_PAGE_H

#define VM_PAGE_H

#include <stddef.h>
#include <inttypes.h>
#include "devices/block.h"
#include "vm/frame.h"

// **need to define how we calculated this number**
#define NUM_SWAP_SLOTS 2048

struct swap_entry
{			

//# Paul drove here.	
	bool empty;     		// set to true if swap slot is taken
	struct page *page;		// Holds page put in swap
//# Paul ends driving

	// block_sector_t first_index;	//Used to access the first sector of this entry's data in swap
	// block_sector_t last_index;	//Used to access the last sector of this entry's data in swap

};

void swap_page(void *upage, void *kpage);
struct page *get_page_from_swap(void *upage);
void remove_page_from_swap(struct page *p);

#endif







