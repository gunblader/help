

struct page
{
	// void *kva; /* The kernel virtual address of this page. i.e. its frame */
	int resident_bit; /* Set 1 if the page is in physical memory, 0 otherwise */
	// int reference_bit; /* Set if it was recently used */
};
