#include "userprog/syscall.h"
#include <stdio.h>
#include <sys/types.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "devices/shutdown.h"
#include "userprog/process.h"
#include "threads/vaddr.h"
#include "pagedir.h"

//typedef int pid_t;

bool verify_user(void *user_esp);
static void syscall_handler (struct intr_frame *);
void halt (void);
void exit (int status);
pid_t exec (const char *cmd_line);
int wait (pid_t pid);
bool create (const char *file, unsigned initial_size);
bool remove (const char *file);
int open (const char *file);
int filesize (int fd);
int read (int fd, void *buffer, unsigned size);
int write (int fd, const void *buffer, unsigned size);
void seek (int fd, unsigned position);
unsigned tell (int fd);
void close (int fd);



void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

// #Kenneth Drove here
bool verify_user(void *user_esp){
  // check if pointer is valid by checking these conditions: 
  // 1. a null pointer
  // 2. a pointer to unmapped virtual memory, 
  // 3. a pointer to kernel virtual address space
  struct thread *cur = thread_current();
  if (lookup_page(cur->pagedir, user_esp, false) == NULL ||
   user_esp == NULL || is_kernel_vaddr(user_esp))
  {
    return false;
  }
  return true;

//  return !(lookup_page(cur->pagedir, user_esp, false) == NULL || user_esp == NULL || is_kernel_vaddr(user_esp));
}


//When syscall_handler gets control, the syscall number is in the 32-bit num at the 
//caller's stack pointer, the first argument is the next 32-bit word and so on.
static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  // grab esp from f
  //#Adam Drove Here
  void *user_esp = f->esp;
  if(!verify_user(user_esp)){
    //terminating the offending process and freeing its resources
    //thread_exit() vs process_exit()?
    thread_exit();
  }

  // #Kenneth drove here
  pid_t exec_pid = -1;
  switch(*(int *)user_esp){
    case SYS_HALT:
      halt();
      break;
    case SYS_EXIT:
      //get the first arg off the stack
      user_esp += 4;
      int status = *(int *)user_esp;
      exit(status);
      break;
    case SYS_EXEC:
      //get the char * off the stack
      user_esp += 4;
      char *cmdLine = (char *)user_esp;
      exec_pid = exec(cmdLine);
      break;
    // #Jacob Drove Here
    case SYS_WAIT:
      user_esp += 4;
      pid_t temp_pid = *(pid_t *)user_esp;
      wait(temp_pid);
      break;


    default:
      break;
  }//END OF SWITCH CASE


  //#Paul drove here
  // //printf ("system call!\n");
  // if(*user_esp == SYS_HALT)
  // {
  //   halt();
  // }
  // //
  // if(*user_esp == SYS_EXIT)
  // {
  //   //need to send status to exit
  //   exit();
  // }
  // if(*user_esp == SYS_EXEC)
  // {
    
  //   pid_t exec (const char *file);

  // }
  // if(*user_esp == SYS_WAIT)
  // {
  //   int wait (pid_t);
  // }
  // if(*user_esp == SYS_CREATE)
  // {
  //   bool create (const char *file, unsigned initial_size);
  // }
  // if(*user_esp == SYS_REMOVE)
  // {

  // }
  // if(*user_esp == SYS_OPEN)
  // {

  // }
  // if(*user_esp == SYS_FILESIZE)
  // {

  // }
  // if(*user_esp == SYS_READ)
  // {

  // }
  // if(*user_esp == SYS_WRITE)
  // {

  // }
  // if(*user_esp == SYS_SEEK)
  // {

  // }
  // if(*user_esp == SYS_TELL)
  // {

  // }
  // if(*user_esp == SYS_CLOSE)
  // {

  // }

  // thread_exit ();
}




/* Terminates Pintos by calling shutdown_power_off() (declared in "devices/shutdown.h"). 
   This should be seldom used, because you lose some information about possible 
   deadlock situations, etc. */
void halt (void)
{
	shutdown_power_off();
}

/*  Terminates the current user program, returning status to the kernel. If the process's
    parent waits for it (see below), this is the status that will be returned. 
    Conventionally, a status of 0 indicates success and nonzero values indicate errors. */
void exit (int status UNUSED)
{

}

/*  Runs the executable whose name is given in cmd_line, passing any given 
    arguments, and returns the new process's program id (pid). Must return pid -1,
    which otherwise should not be a valid pid, if the program cannot load or run for
    any reason. Thus, the parent process cannot return from the exec until it knows 
    whether the child process successfully loaded its executable. You must use
    appropriate synchronization to ensure this.  */
pid_t exec (const char *cmd_line UNUSED)
{
	//#Kenneth Drove here
	process_execute(cmd_line);

	return -1;
}

 /* Waits for a child process pid and retrieves the child's exit status. */
int wait (pid_t pid UNUSED)
{
	return -1;
}

/* Creates a new file called file initially initial_size bytes in size. 
   Returns true if successful, false otherwise. Creating a new file does
   not open it: opening the new file is a separate operation which would 
   require a open system call. */
bool create (const char *file UNUSED, unsigned initial_size UNUSED)
{
	return false;
}

/* Deletes the file called file. Returns true if successful, false otherwise.
   A file may be removed regardless of whether it is open or closed, and removing 
   an open file does not close it. See Removing an Open File, for details */
bool remove (const char *file UNUSED)
{
	return false;
}

/* Opens the file called file. Returns a nonnegative integer handle called
 a "file descriptor" (fd) or -1 if the file could not be opened */
int open (const char *file UNUSED)
{
	return -1;
}

/* Returns the size, in bytes, of the file open as fd */
int filesize (int fd UNUSED)
{
	return -1;
}

/* Reads size bytes from the file open as fd into buffer. Returns the number of bytes
 actually read (0 at end of file), or -1 if the file could not be read 
 (due to a condition other than end of file). fd 0 reads from the keyboard using 
 input_getc() */
int read (int fd UNUSED, void *buffer UNUSED, unsigned size UNUSED){
	return -1;
}

/* Writes size bytes from buffer to the open file fd. Returns the number of bytes
 actually written, which may be less than size if some bytes could not be written. 
 Writing past end-of-file would normally extend the file, but file growth is not 
 implemented by the basic file system. The expected behavior is to write as many bytes 
 as possible up to end-of-file and return the actual number written, or 0 if no bytes 
 could be written at all. fd 1 writes to the console. Your code to write to the console 
 should write all of buffer in one call to putbuf(), at least as long as size is not 
 bigger than a few hundred bytes. (It is reasonable to break up larger buffers.) 
 Otherwise, lines of text output by different processes may end up interleaved on the 
 console, confusing both human readers and our grading scripts. */
int write (int fd UNUSED, const void *buffer UNUSED, unsigned size UNUSED){
	return -1;
}

void seek (int fd UNUSED, unsigned position UNUSED){

}

unsigned tell (int fd UNUSED){
	return 0;
}

void close (int fd UNUSED){

}
