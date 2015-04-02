#include "userprog/pagedir.h"
#include "vm/page.h"
#include "threads/thread.h"

// Jacob & Kenneth Drove Here

unsigned page_hash(const struct hash_elem *e, void *aux);
bool page_less (const struct hash_elem *a_, const struct hash_elem *b_,
           void *aux);

bool init;

void
page_init(){
	init = false;
}

//adds the page to the page table
void
add_page(struct file *file, off_t ofs, uint8_t *vaddr,
    uint32_t read_bytes, uint32_t zero_bytes, bool writable)
{
	//if this is the first time we are adding to the supp pagetable
	//then initialize that hash table here
	struct thread *cur_thread = thread_current();
	if(init == false){
		printf("initialized hash table\n");
		hash_init(&cur_thread->pagetable, page_hash, page_less, NULL);
		init = true;
	}
	struct page *p = malloc(sizeof(struct page));
	p->addr = vaddr;
	p->resident_bit = false;
	p->in_swap = false;
	p->in_filesys = false;

	p->file = file;
	p->ofs = ofs;
	p->read_bytes = read_bytes;
	p->zero_bytes = zero_bytes;
	p->writable = writable;

	printf("Add page at address, 0x%x, to supplemental page table\n", p->addr);
	hash_insert(&cur_thread->pagetable, &p->page_table_elem);
}


/* Taken from the documentation */
unsigned
page_hash(const struct hash_elem *e, void *aux){
  const struct page *p = hash_entry(e, struct page, page_table_elem);
  unsigned hash = hash_bytes(&p->addr, sizeof p->addr);
  printf("Page 0x%x has a hash of %u\n", p->addr, hash);
  return hash;
}

/* Returns true if page a precedes page b.
    THIS IS TAKEN FROM THE DOCUMENTATION */
bool
page_less (const struct hash_elem *a_, const struct hash_elem *b_,
           void *aux)
{
  const struct page *a = hash_entry (a_, struct page, page_table_elem);
  const struct page *b = hash_entry (b_, struct page, page_table_elem);

  return a->addr < b->addr;
}


struct page *
find_page(void *addr){
	struct page p;
	struct hash_elem *h;

	p.addr = addr;
	printf("find_page using addr: 0x%x\n", p.addr);
	h = hash_find(&thread_current()->pagetable, &p.page_table_elem);
	if(h == NULL)
		return NULL;
	else
		return hash_entry(h, struct page, page_table_elem);
}
