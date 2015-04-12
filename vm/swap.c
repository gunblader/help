#include <stddef.h>
#include "vm/page.h"
#include "devices/block.h"
#include "vm/swap.h"


struct swap *swap_table;

// function that initializes the swap table. 
void
swap_init()
{
	// num_swap_slots = 2000;
	swap_table = (struct swap *)malloc(NUM_SWAP_SLOTS * sizeof(struct swap *));

	//# Paul drove here.
	int i;
	for(i = 0; i < NUM_SWAP_SLOTS; i++)
	{
		swap_table[i].taken = false;
		swap_table[i].slot_num = i;
	}
	//# Paul ends driving
}

//# Paul drove here.
void *
swap_page(void *page)
{
	struct block *swap_space = block_get_role(BLOCK_SWAP);
	// int block_sectors = block_size(swap_space);
	// int swap_slots = block_sectors/8;

	struct swap *swap_slot;
	int i;
	int slot_num;
	bool swap_empty = false;
	int bytes_read = 0;

	//find empty swap spot in the swap table
	for(i = 0; i < NUM_SWAP_SLOTS; i++)
	{
		slot_num++;

		block_sector_t s;
		swap_slot = &swap_table[i];

		//Continue looping until you find an empty swap slot
		if(swap_slot->taken == false)
		{
			// write page into all 8 sectors of the swap block that represent that
			// swap slot.
			swap_slot->page_info = page;
			s = slot_num * 8;
			block_write (swap_space, s, page);

			s++;
			bytes_read += 512;
			block_write (swap_space, s, page + bytes_read);

			s++;
			bytes_read += 512;
			block_write (swap_space, s, page + bytes_read);

			s++;
			bytes_read += 512;
			block_write (swap_space, s, page + bytes_read);

			s++;
			bytes_read += 512;
			block_write (swap_space, s, page + bytes_read);

			s++;
			bytes_read += 512;
			block_write (swap_space, s, page + bytes_read);

			s++;
			bytes_read += 512;
			block_write (swap_space, s, page + bytes_read);

			s++;
			bytes_read += 512;
			block_write (swap_space, s, page + bytes_read);

		}
		else
			swap_empty = true;
	}

	if (swap_empty)
		PANIC ("Killed process because Frame table full and swap table full.");

	return swap_slot;
}
//# Paul ends driving

//# Paul drove here.
void *
get_page_from_swap(void *page)
{
	struct block *swap_space = block_get_role(BLOCK_SWAP);
	struct swap *swap_slot;
	int i;
	int slot_num;
	bool swap_empty = false;
	int bytes_read = 0;

	for(i = 0; i < NUM_SWAP_SLOTS; i++)
	{
		slot_num++;

		block_sector_t s;
		swap_slot = &swap_table[i];

		//Continue looping until you find swap slot with page requested
		if(swap_slot->page_info == page)
		{
			s = slot_num * 8;
			block_read (swap_space, s, page);

			s++;
			bytes_read += 512;
			block_read (swap_space, s, page + bytes_read);

			s++;
			bytes_read += 512;
			block_read (swap_space, s, page + bytes_read);

			s++;
			bytes_read += 512;
			block_read (swap_space, s, page + bytes_read);

			s++;
			bytes_read += 512;
			block_read (swap_space, s, page + bytes_read);

			s++;
			bytes_read += 512;
			block_read (swap_space, s, page + bytes_read);

			s++;
			bytes_read += 512;
			block_read (swap_space, s, page + bytes_read);

			s++;
			bytes_read += 512;
			block_read (swap_space, s, page + bytes_read);

		}
	}

}
//# Paul ends driving
