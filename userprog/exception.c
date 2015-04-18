#include "userprog/exception.h"
#include <inttypes.h>
#include <stdio.h>
#include "userprog/gdt.h"
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "vm/frame.h"
#include "threads/vaddr.h"
#include "vm/page.h"

/* Number of page faults processed. */
static long long page_fault_cnt;

static void kill (struct intr_frame *);
static void page_fault (struct intr_frame *);

/* Registers handlers for interrupts that can be caused by user
   programs.

   In a real Unix-like OS, most of these interrupts would be
   passed along to the user process in the form of signals, as
   described in [SV-386] 3-24 and 3-25, but we don't implement
   signals.  Instead, we'll make them simply kill the user
   process.

   Page faults are an exception.  Here they are treated the same
   way as other exceptions, but this will need to change to
   implement virtual memory.

   Refer to [IA32-v3a] section 5.15 "Exception and Interrupt
   Reference" for a description of each of these exceptions. */
void
exception_init (void) 
{
  /* These exceptions can be raised explicitly by a user program,
     e.g. via the INT, INT3, INTO, and BOUND instructions.  Thus,
     we set DPL==3, meaning that user programs are allowed to
     invoke them via these instructions. */
  intr_register_int (3, 3, INTR_ON, kill, "#BP Breakpoint Exception");
  intr_register_int (4, 3, INTR_ON, kill, "#OF Overflow Exception");
  intr_register_int (5, 3, INTR_ON, kill,
                     "#BR BOUND Range Exceeded Exception");

  /* These exceptions have DPL==0, preventing user processes from
     invoking them via the INT instruction.  They can still be
     caused indirectly, e.g. #DE can be caused by dividing by
     0.  */
  intr_register_int (0, 0, INTR_ON, kill, "#DE Divide Error");
  intr_register_int (1, 0, INTR_ON, kill, "#DB Debug Exception");
  intr_register_int (6, 0, INTR_ON, kill, "#UD Invalid Opcode Exception");
  intr_register_int (7, 0, INTR_ON, kill,
                     "#NM Device Not Available Exception");
  intr_register_int (11, 0, INTR_ON, kill, "#NP Segment Not Present");
  intr_register_int (12, 0, INTR_ON, kill, "#SS Stack Fault Exception");
  intr_register_int (13, 0, INTR_ON, kill, "#GP General Protection Exception");
  intr_register_int (16, 0, INTR_ON, kill, "#MF x87 FPU Floating-Point Error");
  intr_register_int (19, 0, INTR_ON, kill,
                     "#XF SIMD Floating-Point Exception");

  /* Most exceptions can be handled with interrupts turned on.
     We need to disable interrupts for page faults because the
     fault address is stored in CR2 and needs to be preserved. */
  intr_register_int (14, 0, INTR_OFF, page_fault, "#PF Page-Fault Exception");
}

/* Prints exception statistics. */
void
exception_print_stats (void) 
{
  printf ("Exception: %lld page faults\n", page_fault_cnt);
}

/* Handler for an exception (probably) caused by a user process. */
static void
kill (struct intr_frame *f) 
{
  /* This interrupt is one (probably) caused by a user process.
     For example, the process might have tried to access unmapped
     virtual memory (a page fault).  For now, we simply kill the
     user process.  Later, we'll want to handle page faults in
     the kernel.  Real Unix-like operating systems pass most
     exceptions back to the process via signals, but we don't
     implement them. */
     
  /* The interrupt frame's code segment value tells us where the
     exception originated. */
  switch (f->cs)
    {
    case SEL_UCSEG:
      /* User's code segment, so it's a user exception, as we
         expected.  Kill the user process.  */
      printf ("%s: dying due to interrupt %#04x (%s).\n",
              thread_name (), f->vec_no, intr_name (f->vec_no));
      intr_dump_frame (f);
      thread_exit (); 

    case SEL_KCSEG:
      /* Kernel's code segment, which indicates a kernel bug.
         Kernel code shouldn't throw exceptions.  (Page faults
         may cause kernel exceptions--but they shouldn't arrive
         here.)  Panic the kernel to make the point.  */
      intr_dump_frame (f);
      PANIC ("Kernel bug - unexpected interrupt in kernel"); 

    default:
      /* Some other code segment?  Shouldn't happen.  Panic the
         kernel. */
      printf ("Interrupt %#04x (%s) in unknown segment %04x\n",
             f->vec_no, intr_name (f->vec_no), f->cs);
      thread_exit ();
    }
}

/* Page fault handler.  This is a skeleton that must be filled in
   to implement virtual memory.  Some solutions to project 2 may
   also require modifying this code.

   At entry, the address that faulted is in CR2 (Control Register
   2) and information about the fault, formatted as described in
   the PF_* macros in exception.h, is in F's error_code member.  The
   example code here shows how to parse that information.  You
   can find more information about both of these in the
   description of "Interrupt 14--Page Fault Exception (#PF)" in
   [IA32-v3a] section 5.15 "Exception and Interrupt Reference". */
static void
page_fault (struct intr_frame *f) 
{
  bool not_present;  /* True: not-present page, false: writing r/o page. */
  bool write;        /* True: access was write, false: access was read. */
  bool user;         /* True: access by user, false: access by kernel. */
  void *fault_addr;  /* Fault address. */

  /* Obtain faulting address, the virtual address that was
     accessed to cause the fault.  It may point to code or to
     data.  It is not necessarily the address of the instruction
     that caused the fault (that's f->eip).
     See [IA32-v2a] "MOV--Move to/from Control Registers" and
     [IA32-v3a] 5.15 "Interrupt 14--Page Fault Exception
     (#PF)". */
  asm ("movl %%cr2, %0" : "=r" (fault_addr));

  /* Turn interrupts back on (they were only off so that we could
     be assured of reading CR2 before it changed). */
  intr_enable ();

  /* Count page faults. */
  page_fault_cnt++;

  /* Determine cause. */
  not_present = (f->error_code & PF_P) == 0;
  write = (f->error_code & PF_W) != 0;
  user = (f->error_code & PF_U) != 0;

  /* To implement virtual memory, delete the rest of the function
     body, and replace it with code that brings in the page to
     which fault_addr refers */
  // printf ("Page fault at %p: %s error %s page in %s context.\n",
  //         fault_addr,
  //         not_present ? "not present" : "rights violation",
  //         write ? "writing" : "reading",
  //         user ? "user" : "kernel");

  // printf("There is no crying in Pintos!\n");

  // debug_backtrace();

  // kill (f);
     printf("*******************************\n");
     printf("faulting address: 0x%x\n", fault_addr);

  /* If the supplemental page table indicates that the user process
      should not expect any data at the address it was trying to access,
      or if the page lies within kernel virtual memory, or if the access
      is an attempt to write to a read-only page, then the access is invalid
      and we should terminate the process */

  if(fault_addr == NULL || (user && is_kernel_vaddr(fault_addr)) || !not_present)
  {
    printf("Terminating process\n");
    thread_exit();
  }

  struct thread *cur_thread = thread_current();

  //search for the fault_addr in the current threads page table
  //faulting page = fp
  struct page *fp = find_page(pg_round_down(fault_addr));

  //if the page was not found, do something
  if(fp == NULL)
  {

    if(!write){
      printf("Exit in PF #2\n");
      // thread_exit();
      exit(-1);
    }
    //either terminate the process or grow the stack
    // printf("The faulting page wasn't found in the supp page table\n");
    // printf("Faulting addr: 0x%x\n", fault_addr);
    
    // if(cur_thread == kernel)
    //  syscall_saved_esp
    // else
    //  use f->esp
    
    uint32_t cur_esp = (!user) ? cur_thread->cur_esp : f-> esp;
    //if we are 32 or 4 bytes below the stack pointer, then grow the stack
    int diff = (void *)cur_esp - fault_addr;
    // might need to change to 40 - Sage
    if(diff <= 32){
      printf("Growing Stack\n");
      //add a stack page to the supplemental page table and install it
      
      // printf("GROW THE STACK HERE\n");

      // #Jacob and Paul Drove Here
       uint8_t *kpage;
       int *alloc_stack_space = f->esp;

       // we need to get a new frame and put a new page in it, so that we can
       // allocate more space on the stack inside of the frame.
       struct frame *f = get_frame();
       kpage = f->kva;
       
       // create a page struct for the page struct for the page that is put into
       // the frame table. This is so that we can add the page to the supplemental
       // page table

       // add_page(cur_thread->file, 0, kpage, 0, 0, true);

       if(!add_page_to_stack(f, pg_round_down(fault_addr)))
       {
          printf("Error installing stack page\n");
          // thread_exit();
          exit(-1);
       }

      return;
    
       // #End Jacob and Paul driving

    }
    else{
      // will have to input the correct failing behavior for output match here.
      // thread_exit();
      printf("LALALALALALALA\n");
      exit(-1);
    }
  }
  //else, connect the addresses
  else
  {
    //check and see if the faulting page is in a swap slot
    struct frame *f = get_frame();

    if(fp->in_swap)
    { 
      // printf("IN swap check\n");
      // bring it back in and place it in an empty frame;
      f->cur_page = get_page_from_swap(fp->addr, f->kva);
      printf("Page brought in from swap: 0x%x\n", f->cur_page->addr);
            printf("*******************************\n");


      // fp->addr = fault_addr;
      // update  the page directory
      if(!(pagedir_get_page (cur_thread->pagedir, f->cur_page->addr) == NULL
        && pagedir_set_page (cur_thread->pagedir, f->cur_page->addr, f->kva, fp->writable)))
      {
        printf("IN swap: failed install\n");
        return;
      }
        //set dirty bit to 1
        // pagedir_set_dirty(cur_thread->pagedir, fault_addr, 1);

    }
    else
    {
      // printf("Reading in page from the file\n");
      size_t page_read_bytes = fp->read_bytes < PGSIZE ? fp->read_bytes : PGSIZE;
      size_t page_zero_bytes = PGSIZE - page_read_bytes;
      
      //obtain a frame to store the page
      
      //#paul drove here.
      uint8_t *kpage;
      // struct frame *f = get_frame();
      f->cur_page = fp;
      kpage = f->kva;
      //driving ends.

      if(kpage == NULL)
      {
        printf("Kpage was null\n");
        return;
      }

      //fetch the data into the frame, by reading it from filesys or swap, zeroing it, etc.
      file_seek(fp->file, fp->ofs);
      if(file_read(fp->file, kpage, page_read_bytes) != (int) page_read_bytes){
        printf("File wasn't read properly\n");
        // palloc_free_page (kpage);
        return;
      }
      memset (kpage + page_read_bytes, 0, page_zero_bytes);

      //point the PTE for the faulty address to the physical page
      if(!(pagedir_get_page (cur_thread->pagedir, fp->addr) == NULL
        && pagedir_set_page (cur_thread->pagedir, fp->addr, kpage, fp->writable)))
      {
        printf("Page wasn't mapped properly\n");
        // palloc_free_page (kpage);
        return;
      }
    }


  }
}

