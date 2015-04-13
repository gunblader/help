#include <stddef.h>
#include "vm/page.h"
#include "devices/block.h"
#include "vm/swap.h"

#define SIZE_OF_SECTORS 512

struct swap *swap_table;

//helper function to write a page to swap space
void write_to_swap(struct block *swap_space, block_sector_t sector_to_write, void *page);

// helper function to write data from swap space to frames
void read_page_from_swap(struct block *swap_space, block_sector_t sector_to_write, void *page);


// function that initializes the swap table. 
void
swap_init()
{
	// num_swap_slots = 2000;
	swap_table = (struct swap *)malloc(NUM_SWAP_SLOTS * sizeof(struct swap *));

	//# Paul drove here
	int i;
	for(i = 0; i < NUM_SWAP_SLOTS; i++)
	{
		swap_table[i].taken = false;
		swap_table[i].slot_num = i;
	}
	// #End of Paul driving
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
	int slot_num = 0;
	bool swap_empty = false;
	int bytes_read = 0;

	//find empty swap spot in the swap table
	for(i = 0; i < NUM_SWAP_SLOTS; i++)
	{
		block_sector_t s;
		swap_slot = &swap_table[i];

		//Continue looping until you find an empty swap slot
		if(!swap_slot->taken)
		{
			// write page into all 8 sectors of the swap block that represent that
			// swap slot.
			swap_slot->page_info = page;
			s = slot_num * 8;

			write_to_swap(swap_space, s, page);

			break;
		}
		else
			swap_empty = true;

		slot_num++;
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
	struct swap *swap_slot = NULL;
	int i;
	int slot_num = 0;
	bool swap_empty = false;
	int bytes_read = 0;

	for(i = 0; i < NUM_SWAP_SLOTS; i++)
	{

		block_sector_t s;
		swap_slot = &swap_table[i];

		//Continue looping until you find swap slot with page requested
		if(swap_slot->page_info == page)
		{
			s = slot_num * 8;
			
			//read data from swap into page
			read_page_from_swap(swap_space, s, page);

			return;

		}
		slot_num++;
	}

}
//# Paul ends driving

// # Adam driving
//helper function to write data from user page to swap sectors
void 
write_to_swap(struct block *swap_space, block_sector_t sector_to_write, void *page)
{
	int count = 0;
	while (count < 8)
	{
		block_write (swap_space, sector_to_write, page + bytes_read);
		sector_to_write++;
		bytes_read += SIZE_OF_SECTORS;
		count++;
	}
}

//helper function to write data from swap sectors to given page
void 
read_page_from_swap(struct block *swap_space, block_sector_t sector_to_write, void *page)
{
	int count = 0;
	while (count < 8)
	{
		block_read (swap_space, sector_to_write, page + bytes_read);
		sector_to_write++;
		bytes_read += SIZE_OF_SECTORS;
		count++;
	}
}
// #End Adam driving

