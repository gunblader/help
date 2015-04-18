#include <stddef.h>
#include "vm/page.h"
#include "devices/block.h"
#include "vm/swap.h"

#define SIZE_OF_SECTORS 512

/* 
	Global swap table to keep track of all swap slots that are available for frames
	to be swapped into 
*/
struct swap_entry *swap_table;
struct block *swap_space;

block_sector_t find_empty_sector();

//helper function to write a page to swap space
void write_to_swap(struct block *swap_space, block_sector_t sector_to_write, 
	void *page);

// helper function to write data from swap space to frames
void read_page_from_swap(struct block *swap_space, 
	block_sector_t sector_to_write, void *page);


// function that initializes the swap table. 
void
swap_init()
{
	swap_table = (struct swap_entry *)malloc(NUM_SWAP_SLOTS * sizeof(struct swap_entry));
	//this is assuming swap_space contains all of the swap space
	swap_space = block_get_role(BLOCK_SWAP);
	if(swap_space == NULL)
	{
		PANIC("Called this before the OS initialized the block device\n");
	}

	int i;
	for(i = 0; i < NUM_SWAP_SLOTS; i++)
	{
		struct swap_entry *slot = &swap_table[i]; 
		slot->slot_num = i;
		slot->empty = true;
		slot->page = NULL;
	}
}

//loop through our swap_table and return the index of the first free sector
block_sector_t
find_empty_sector(){
	bool found_empty = false;

	//loop through for empty spot
	int i;
	for(i = 0; i < NUM_SWAP_SLOTS; i++)
	{
		struct swap_entry *slot = &swap_table[i];
		if(slot->empty){
			found_empty = true;
			break;
		}
	}

	//if you can't find an empty spot panic the kernel
	if(!found_empty)
	{
		PANIC("Swap Table is Full");
	}

	return i * 8;
}

/*Places page "upage" into swap and updates the pagedir.
  Also, stores the block_sector_t inside of the page struct
  of where the page is in swap*/
void
swap_page(void *upage, void *kpage){
	printf("IN SWAP PAGE\n");
	//find the page to swap in our supplemental page table
	struct page *page_to_swap = find_page(upage);
	ASSERT(page_to_swap != NULL);

	//find the next empty sector to store this page
	block_sector_t next_empty = find_empty_sector();

	//The page stores its location in swap
	page_to_swap->first_sector = next_empty;

	//write to the swap_space
	write_to_swap(swap_space, next_empty, kpage);

	page_to_swap->in_swap = true;

	//update swap table
	struct swap_entry *slot = &swap_table[next_empty / 8];
	slot->empty = false;
	slot->page = page_to_swap;

	printf("New swap entry: \n\tslot_num: %i\n\tempty: %i\n\tupage: 0x%x\n", 
		slot->slot_num, slot->empty, slot->page->addr);
	printf("EXITED SWAP PAGE\n");
}

/* Returns the page given by "upage" from swap*/
struct page *
get_page_from_swap(void *upage, void *kpage){
	printf("****IN GET PAGE FROM SWAP****\n");
	//find the page in the supplemental page table
	struct page *page_from_swap = find_page(upage);
	ASSERT(page_from_swap->in_swap);

	//get the location of this page in swap
	block_sector_t first_sector = page_from_swap->first_sector;

	//read the page in from swap
	
	read_page_from_swap(swap_space, first_sector, kpage);


	struct swap_entry *slot = &swap_table[first_sector / 8];

	
	printf("index: %d\n", first_sector/8);
	ASSERT(slot->page != NULL);
	printf("swap entry: \n\tslot_num: %i\n\tempty: %i\n\tupage: 0x%x\n", 
		slot->slot_num, slot->empty, slot->page->addr);
	slot->empty = true;
	slot->page = NULL;

	printf("Updated swap entry: \n\tslot_num: %i\n\tempty: %i\n\t\n", 
		slot->slot_num, slot->empty);

	// printf("EXITED GET PAGE FROM SWAP\n");
	return page_from_swap;
}

void 
remove_page_from_swap(struct page *p){
	struct swap_entry *slot = &swap_table[p->first_sector / 8];
	slot->empty = true;
	slot->page = NULL;
}

// # Adam driving
//helper function to write data from user page to swap sectors
void 
write_to_swap(struct block *swap_space, block_sector_t sector_to_write, 
	void *page)
{
	int count = 0;
	int bytes_written = 0;
	while (count < 8)
	{
		block_write (swap_space, sector_to_write, page + bytes_written);
		sector_to_write++;
		bytes_written += SIZE_OF_SECTORS;
		count++;
	}
}

//helper function to write data from swap sectors to given page
void 
read_page_from_swap(struct block *swap_space, block_sector_t sector_to_write, 
	void *page)
{
	int count = 0;
	int bytes_read = 0;
	while (count < 8)
	{
		block_read (swap_space, sector_to_write, page + bytes_read);
		sector_to_write++;
		bytes_read += SIZE_OF_SECTORS;
		count++;
	}
}
// #End Adam driving

