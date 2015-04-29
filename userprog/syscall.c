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
#include <list.h>
#include "filesys/file.h"


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
struct file_info *get_file_from_fd(int fd);

// #Jacob Drove Here:
// Create a global lock to provide mutual exclusion for the
// utilization of the file system.
struct lock lock;
int global_fd;
static struct list file_list;
// #End Jacob driving



void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  // #Jacob Drove Here
  lock_init(&lock);
  list_init(&file_list);
  global_fd = 1;
  // #End Jacob Driving
}

// #Kenneth Drove here
void verify_user(void *user_esp){
  // check if pointer is valid by checking these conditions: 
  // 1. a null pointer
  // 2. a pointer to unmapped virtual memory, 
  // 3. a pointer to kernel virtual address space
  struct thread *cur = thread_current();

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

  int *user_esp = (int *)f->esp;
  
  verify_user(user_esp);

  // #Kenneth drove here
  char *file;
  void *buffer;
  unsigned size;
  int fd;
  unsigned position;
  bool result;
  char *argv;
  switch(*user_esp){
    case SYS_HALT:
      halt();
      break;
    case SYS_EXIT:
      user_esp++;
      verify_user(user_esp);
      int status = *user_esp;

      exit(status);
      break;
    
    case SYS_EXEC:
      user_esp++;
      verify_user(user_esp);
      char *cmdLine = *(int *)user_esp;
      verify_user(cmdLine);
      f->eax = exec(cmdLine);
      break;
    
    // #Jacob Drove Here
    case SYS_WAIT:
      user_esp++;
      verify_user(user_esp);
      pid_t temp_pid = *(pid_t *)user_esp;
      f->eax = wait(temp_pid);
      break;
    
    //#Kenneth Drove here
    case SYS_CREATE:
      user_esp++;
      verify_user(user_esp);
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
      file = *(int *)user_esp;
      verify_user(file);
      f->eax = remove(file);
      break;
    
    case SYS_OPEN:
      user_esp++;
      verify_user(user_esp);
      file = *(int *)user_esp;
      verify_user(file);
      f->eax = open(file);
      break;
      
      // #Adam driving here
    case SYS_FILESIZE:
      user_esp++;
      verify_user(user_esp);
      fd = *(int *)user_esp;
      f->eax = filesize(fd);
      break;
    
    case SYS_READ:
      user_esp++;
      verify_user(user_esp);
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
      verify_user(user_esp);
      fd = *(int *)user_esp;
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
      fd = *(int *)user_esp;
      user_esp++;
      verify_user(user_esp);
      position = *(unsigned *)user_esp;
      seek(fd, position);
      break;
    case SYS_TELL:
      user_esp++;
      verify_user(user_esp);
      fd = *(int *)user_esp;
      f->eax = tell (fd);
      break;
    case SYS_CLOSE:
      user_esp++;
      verify_user(user_esp);
      fd = *(int *)user_esp;
      close (fd);
      break;

    /* Directory System calls */
    case SYS_CHDIR:
      user_esp++;
      verify_user(user_esp);
      char *dir = *(int *)user_esp;
      verify_user(dir);

      f->eax = chdir(dir);
      break;
    case SYS_MKDIR:
      user_esp++;
      verify_user(user_esp);
      char *dir = *(int *)user_esp;
      verify_user(dir);

      f->eax = mkdir(dir);
      break;
    case SYS_READDIR:
      user_esp++;
      verify_user(user_esp);
      fd = *(int *)user_esp;
      user_esp++;
      verify_user(user_esp);
      char *name = *(int *)user_esp;
      verify_user(name);

      f->eax = readdir(fd, name);
      break;
    case SYS_ISDIR:
      user_esp++;
      verify_user(user_esp);
      fd = *(int *)fd;

      f->eax = isdir(fd);
      break;
    case SYS_INUMBER:
      user_esp++;
      verify_user(user_esp);
      fd = *(int *)user_esp;

      f->eax = inumber(fd);
      break;


    default:

      break;
  }//END OF SWITCH CASE

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
  // #Kenneth drove here
  //set the status of the child to be returned to the parent
  thread_current()->exit_status = status;
  //clear the page of the child process
  thread_current()->called_exit = true;

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
  // printf("\n\nIN EXEC: cmd_line - %s\n\n", cmd_line);
  struct thread *parent = thread_current();
  pid_t pid = process_execute(cmd_line);

  return pid;
}

 /* Waits for a child process pid and retrieves the child's exit status. */
int wait (pid_t pid)
{
  // printf("\n\n %s Called Wait\n\n", thread_current()->name);
  struct thread *parent = thread_current();
  struct thread *child = get_thread(pid);
  int status;
  status = process_wait(pid);
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
  struct list_elem *e = NULL;
  struct file_info *cur_file = NULL;
  for(e = list_begin(&file_list); e != list_end(&file_list); e = list_next(e)){
    struct file_info *temp = list_entry(e, struct file_info, file_list_elem);
    if(file_name == temp->name){
      cur_file = temp;
      break;
    }
  }
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
  if(file_name == NULL)
    return -1;
  lock_acquire(&lock);
  
  struct file *file = NULL;
  struct file_info *f = palloc_get_page(0);
  if(f == NULL) {
    lock_release(&lock);
    return -1;
  }
  file = filesys_open(file_name);
  if(file == NULL){
    lock_release(&lock);
    return -1;
  }
  
  f->fd = ++global_fd;
  f->file = file;
  f->name = file_name;
  list_push_back(&file_list, &f->file_list_elem);
  list_push_back(&thread_current()->fd_list, &f->thread_file_list_elem);

  lock_release(&lock);
	return f->fd;
}

/* Returns the size, in bytes, of the file open as fd */
int filesize (int fd)
{
  // #Adam driving here
  lock_acquire(&lock);

  struct file_info *cur_file = get_file_from_fd(fd);
  if(cur_file == NULL){
    lock_release(&lock);
    return -1;
  }

  off_t size;
  size = file_length(cur_file->file);

  lock_release(&lock);
	return size;
}

/* Reads size bytes from the file open as fd into buffer. Returns the number of bytes
 actually read (0 at end of file), or -1 if the file could not be read 
 (due to a condition other than end of file). fd 0 reads from the keyboard using 
 input_getc() */
 int read (int fd, void *buffer, unsigned size){

  // #Kenneth and Jacob Drove Here
  lock_acquire(&lock);
  int bytes_read = -1;
  if(fd == 0){
    bytes_read = input_getc();
    lock_release(&lock);
    return -1;
  }

  struct file_info *cur_file_info = get_file_from_fd(fd);
  if(cur_file_info == NULL){
    lock_release(&lock);
    return -1;
  }
  bytes_read = file_read(cur_file_info->file, buffer, size);

  lock_release(&lock);
  // #End Kenneth and Jacob Driving
  return bytes_read;
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
    putbuf(buf, size);
    lock_release(&lock);
    return 0;
  }

  //otherwise write to the open file fd
  struct file_info *cur_file_info = get_file_from_fd(fd);
  if(cur_file_info == NULL){
    lock_release(&lock);
    return 0;
  }
  off_t bytes_written = file_write(cur_file_info->file, buffer, size);
  lock_release(&lock);
  // printf("\n***IN WRITE: bytes_written: %d\n", bytes_written);
	return (int)bytes_written;
}

// # Kenneth, Jacob and Paul Drove here
void seek (int fd, unsigned position){
  lock_acquire(&lock);
  struct file_info *cur_file_info = get_file_from_fd(fd);
  if(cur_file_info == NULL){
    lock_release(&lock);
    return;
  }
  file_seek(cur_file_info->file, (off_t)position);
  lock_release(&lock);
}

unsigned tell (int fd){
  lock_acquire(&lock);
  struct file_info *cur_file_info = get_file_from_fd(fd);
  if(cur_file_info == NULL){
    lock_release(&lock);
    return -1;
  }

  int pos = file_tell(cur_file_info->file);
  lock_release(&lock);
  return pos;
}

void close (int fd){
  lock_acquire(&lock);

  struct file_info *cur_file_info = get_file_from_fd(fd);
  if(cur_file_info == NULL){
    lock_release(&lock);
    return;
  }
  // list_remove(&cur_file_info->file_list_elem);
  list_remove(&cur_file_info->thread_file_list_elem);

  lock_release(&lock);
}

struct file_info *
get_file_from_fd(int fd){
  struct list_elem *e = NULL;
  struct file_info *cur_file_info = NULL;
  //find the current file info if it exists
  for(e = list_begin(&file_list); e != list_end(&file_list); e = list_next(e)){
    struct file_info *temp = list_entry(e, struct file_info, file_list_elem);
    if(temp->fd == fd){
      cur_file_info = temp;
      break;
    }
  }

  return cur_file_info;
}
// #Kenneth, Jacob and Paul finished driving here

/* Directory System Calls */

/* Changes the current working directory of the process to dir,
 which may be relative or absolute. Returns true if successful, false on failure.*/
bool
chdir (const char *dir)
{

}

/* Creates the directory named dir, which may be relative or absolute. Returns true if successful,
 false on failure. Fails if dir already exists or if any directory name in dir, besides the last, 
 does not already exist. That is, mkdir("/a/b/c") succeeds only if "/a/b" already exists and 
 "/a/b/c" does not. */
bool 
mkdir (const char *dir)
{

}

/* Reads a directory entry from file descriptor fd, which must represent a directory. If successful, 
stores the null-terminated file name in name, which must have room for READDIR_MAX_LEN + 1 bytes, 
and returns true. If no entries are left in the directory, returns false. */
bool
readdir (int fd, char *name) 
{

}

/* Returns true if fd represents a directory, false if it represents an ordinary file.  */
bool
isdir (int fd) 
{

}

/* Returns the inode number of the inode associated with fd, which may represent an 
ordinary file or a directory. */
int
inumber (int fd)
{

}
