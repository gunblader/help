#ifdef VM_PAGE_H
#define VM_PAGE_H

#include <stddef.h>
#include <inttypes.h>
#include "devices/block.h"
#include "vm/frame.h"
#define NUM_SWAP_SLOTS 2000

struct swap
{			

//# Paul drove here.	
	bool taken;     // set to true if swap slot is taken
	int slot_num;   // slot number in the swap table

	struct page page_info;    // Holds info about page put in swap

//# Paul ends driving
};

void *swap_page(void *page);
void *get_page_from_swap(void *page);

// size_t num_swap_slots;

#endif







