

struct page
{
	int resident_bit; /* Set 1 if the page is in physical memory, 0 otherwise */
	int reference_bit; /* Set if it was recently used */
};
