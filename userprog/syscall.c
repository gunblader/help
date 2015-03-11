#include "userprog/syscall.h"
#include <stdio.h>
// #include <sys/types.h>
#include <syscall-nr.h>
#include "lib/string.h"
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "devices/shutdown.h"
#include "userprog/process.h"
#include "threads/vaddr.h"
#include "pagedir.h"
#include "filesys/filesys.h"

typedef int pid_t;

void verify_user(void *user_esp);
bool check_num_args(int argc, int expected);
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

// #Jacob Drove Here:
// Create a global lock to provide mutual exclusion for the
// utilization of the file system.
struct lock lock;
int global_fd = 1;
static struct list file_list;
// #End Jacob driving



void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  // #Jacob Drove Here
  lock_init(&lock);
  list_init(&file_list);
  // #End Jacob Driving
}

// #Kenneth Drove here
void verify_user(void *user_esp){
  // check if pointer is valid by checking these conditions: 
  // 1. a null pointer
  // 2. a pointer to unmapped virtual memory, 
  // 3. a pointer to kernel virtual address space
  struct thread *cur = thread_current();
  // if (pagedir_get_page(cur->pagedir, user_esp) == NULL ||
  //  user_esp == NULL || is_kernel_vaddr(user_esp))
  // {
  //   return false;
  // }

  // return true;
  // printf("Checking thread %s in verify_user. pagedir: %x\n", cur->name, cur->pagedir);

  if(user_esp == NULL || !is_user_vaddr(user_esp) ||
  pagedir_get_page(cur->pagedir, user_esp) == NULL){
    thread_exit();
  }
}
// Explain here what this does in a comment
bool check_num_args(int argc, int expected){
  return argc-1 < expected ? true: false;
}


//When syscall_handler gets control, the syscall number is in the 32-bit num at the 
//caller's stack pointer, the first argument is the next 32-bit word and so on.
static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  // grab esp from f
  //#Adam Drove Here
  // lock_acquire(&lock_handler);

  int *user_esp = (int *)f->esp;
  
  // hex_dump (f->esp, f->esp, PHYS_BASE-(f->esp), 1);
  verify_user(user_esp);

  // #Kenneth drove here
  char *file;
  void *buffer;
  unsigned size;
  int fd;
  unsigned position;
  bool result;
  char *argv;
  // printf("System call #: %i\n", *(int *)user_esp);
  switch(*user_esp){
    case SYS_HALT:
      // printf("Called Halt.\n");
      halt();
      break;
    case SYS_EXIT:
      //get argc off stack
      // printf("Called Exit. %s\n", thread_current()->name);
      user_esp++;
      verify_user(user_esp);
      //check number of args
      // result = check_num_args(*user_esp, 1);
      // if(result)
      // {
      //   printf("Not right amount of args\n");
      //   exit(0);
      //   // thread_exit()
      //   return;
      // }
      // printf("outside of if\n");
      //user_esp++;
      //verify_user(user_esp);
      int status = *user_esp;
      //printf("status: %i\n", status);

      exit(status);
      break;
    
    case SYS_EXEC:
    // printf("Called Exec.%s\n", thread_current()->name);
      //get the char * off the stack
      user_esp++;
      // printf("EXEC argc: %s\n", *(int *)user_esp);
      verify_user(user_esp);
      // result = check_num_args(*user_esp, 1);
      // if(result)
      //   return;
      // user_esp++;
      // verify_user(user_esp);
      // //this is argv[0]
      // argv = *user_esp;
      // //this is argv[1]
      // argv++;
      // verify_user(*argv);
      char *cmdLine = *(int *)user_esp;
      verify_user(cmdLine);
      f->eax = exec(cmdLine);
      break;
    
    // #Jacob Drove Here
    case SYS_WAIT:
      // printf("Called Wait. %s\n", thread_current()->name);
      user_esp++;
      verify_user(user_esp);
      // check_num_args(*user_esp, 1);
      pid_t temp_pid = *(pid_t *)user_esp;
      f->eax = wait(temp_pid);
      break;
    
    //#Kenneth Drove here
    case SYS_CREATE:
      user_esp++;
      verify_user(user_esp);
      // check_num_args(*user_esp, 2);
      file = *(int *)user_esp;
      verify_user(file);

      user_esp++;
      verify_user(user_esp);
      size = *(unsigned *)user_esp;
      f->eax = create(file, size);
      break;
    
    case SYS_REMOVE:
      user_esp++;
      verify_user(user_esp);
      // check_num_args(*user_esp, 1);


      file = *(int *)user_esp;
      verify_user(file);
      f->eax = remove(file);
      break;
    
    case SYS_OPEN:
      user_esp++;
      verify_user(user_esp);
      // check_num_args(*user_esp, 1);

      file = *(char *)user_esp;
      f->eax = open(file);
      break;
      
      // #Adam driving here
    case SYS_FILESIZE:
      user_esp++;
      verify_user(user_esp);
      // check_num_args(*user_esp, 1);


      fd = *(int *)user_esp;
      f->eax = filesize(fd);
      break;
    
    case SYS_READ:
      user_esp++;
      verify_user(user_esp);
      // check_num_args(*user_esp, 3);
      fd = *(int *)user_esp;
      user_esp++;
      verify_user(user_esp);
      buffer = *(int *)user_esp;
      verify_user(buffer);
      user_esp++;
      verify_user(user_esp);
      size = *(unsigned *)user_esp;
      f->eax = read(fd, buffer, size);
      break;

    case SYS_WRITE:
      user_esp++;
      // printf("WRITE argc: %i\n", *(int *)user_esp);
      verify_user(user_esp);
      fd = *(int *)user_esp;
      //this is argv[2]
      user_esp++;
      verify_user(user_esp);
      buffer = *user_esp;
      verify_user(buffer);
      user_esp++;
      verify_user(user_esp);
      size = *(int *)user_esp;

      f->eax = write(fd, buffer, size);
      break;
    case SYS_SEEK:
      user_esp++;
      verify_user(user_esp);
      // check_num_args(*user_esp, 2);

      fd = *(int *)user_esp;
      user_esp++;
      verify_user(user_esp);
      position = *(unsigned *)user_esp;
      seek(fd, position);
      break;
    case SYS_TELL:
      user_esp++;
      verify_user(user_esp);
      // check_num_args(*user_esp, 1);

      fd = *(int *)user_esp;
      f->eax = tell (fd);
      break;
    case SYS_CLOSE:
      user_esp++;
      verify_user(user_esp);
      // check_num_args(*user_esp, 1);

      fd = *(int *)user_esp;
      close (fd);
      break;
    default:

      break;
  }//END OF SWITCH CASE

  // thread_exit ();
  // lock_release(&lock_handler);
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
void exit (int status)
{
  // printf("<1>\n");
  //#Paul drove here  
  // int argvsize = strlen(thread_current()->name);
  // char term_message[40];
  // strlcpy(term_message, thread_current()->name, argvsize + 1);
  // strlcat(term_message, ": exit(", 40);
  // strlcat(term_message, , 40);
  // strlcat(term_message, ")\n", 40);
  // argvsize = strlen(term_message);
  // // printf("status: %i\n", status);
  // write(1, term_message, argvsize);

  // #Kenneth drove here
  //set the status of the child to be returned to the parent
  thread_current()->exit_status = status;
  //clear the page of the child process
  thread_current()->called_exit = true;
  // printf("<2>\n");
  // process_exit();
  thread_exit();
}

/*  Runs the executable whose name is given in cmd_line, passing any given 
    arguments, and returns the new process's program id (pid). Must return pid -1,
    which otherwise should not be a valid pid, if the program cannot load or run for
    any reason. Thus, the parent process cannot return from the exec until it knows 
    whether the child process successfully loaded its executable. You must use
    appropriate synchronization to ensure this.  */
pid_t exec (const char *cmd_line)
{
	//#Jacob Drove here
  // SYNCHRONIZATION MUST BE IMPLEMENTED HERE
  // printf("\n\nIN EXEC: cmd_line - %s\n\n", cmd_line);
  struct thread *parent = thread_current();
  pid_t pid = process_execute(cmd_line);
  // printf("\n\n FINISHED process_execute\n\n");
  // sema_down(&thread_current()->sema_thread_create);
  if(pid != TID_ERROR)
    parent->entered_exec = true;
  //lets the parent know that the child is done.

  return pid;
}

 /* Waits for a child process pid and retrieves the child's exit status. */
int wait (pid_t pid)
{
  //waits for the child to be fully set up.
  // printf("Called Wait\n");
  struct thread *parent = thread_current();
  struct thread *child = get_thread(pid);
  int status;
  status = process_wait(pid);
  /* maybe sema_up here and sema down inside thread_exit() before we
     schedule and destroy the child thread. */
  // struct thread *child = get_thread(pid);
  if(child != NULL)
    sema_up(&child->pause_thread_exit);

  return status;

}

/* Creates a new file called file initially initial_size bytes in size. 
   Returns true if successful, false otherwise. Creating a new file does
   not open it: opening the new file is a separate operation which would 
   require a open system call. */
bool create (const char *file_name, unsigned initial_size)
{
  // #Adam Drove here
  lock_acquire(&lock);
  bool result = false;
  result = filesys_create(file_name, initial_size);
  lock_release(&lock);
	return result;
}

/* Deletes the file called file. Returns true if successful, false otherwise.
   A file may be removed regardless of whether it is open or closed, and removing 
   an open file does not close it. See Removing an Open File, for details */
bool remove (const char *file_name)
{
  // #Adam Drove here
  lock_acquire(&lock);
  bool result = false;
  result = filesys_remove(file_name);
  lock_release(&lock);
	return result;
}

/* Opens the file called file. Returns a nonnegative integer handle called
 a "file descriptor" (fd) or -1 if the file could not be opened */
int open (const char *file_name)
{
  // #Adam Drove here
  lock_acquire(&lock);
  struct file *file;
  struct file_info *f;
  file = filesys_open(file_name);
  global_fd++;
  // f->fd = global_fd;
  f->fd = global_fd;
  // list_push_back(&file_list, &f->file_list_elem);
  lock_release(&lock);
	return global_fd;
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
int read (int fd, void *buffer, unsigned size){

  // #Jacob Drove Here
  lock_acquire(&lock);
  


  lock_release(&lock);
  // #End Jacob Driving
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
int write (int fd, const void *buffer, unsigned size){

  ASSERT(buffer != NULL);
  // #Jacob Drove Here
  lock_acquire(&lock);
  char *buf = (char *)buffer;

  if(fd == 1){
    //if size > 256 we need to call putbuf multiple times until buffer has been printed
    // unsigned char_remaining = size;
    // if(size > 256){

    // } 
    putbuf(buf, size);
  }

  lock_release(&lock);
	return -1;
}

void seek (int fd UNUSED, unsigned position UNUSED){

}

unsigned tell (int fd UNUSED){
	return 0;
}

void close (int fd UNUSED){

}
