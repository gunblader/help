#include <stddef.h>
#include "vm/page.h"
#include "devices/block.h"
#include "vm/swap.h"


struct swap *swap_table;

// function that initializes the swap table. 
void
swap_init(){
	// num_swap_slots = 2000;
	swap_table = (struct swap *)malloc(NUM_SWAP_SLOTS * sizeof(struct swap *));
}






