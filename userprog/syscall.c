#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

static void syscall_handler (struct intr_frame *);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  printf ("system call!\n");
  thread_exit ();
}

// /* Terminates Pintos by calling shutdown_power_off() (declared in "devices/shutdown.h"). 
//    This should be seldom used, because you lose some information about possible 
//    deadlock situations, etc. */
// void halt (void)
// {

// }

// /*  Terminates the current user program, returning status to the kernel. If the process's
//     parent waits for it (see below), this is the status that will be returned. 
//     Conventionally, a status of 0 indicates success and nonzero values indicate errors. */
// void exit (int status)
// {

// }

//   Runs the executable whose name is given in cmd_line, passing any given 
//     arguments, and returns the new process's program id (pid). Must return pid -1,
//     which otherwise should not be a valid pid, if the program cannot load or run for
//     any reason. Thus, the parent process cannot return from the exec until it knows 
//     whether the child process successfully loaded its executable. You must use
//     appropriate synchronization to ensure this. 
// pid_t exec (const char *cmd_line)
// {
// 	return NULL;
// }

//  /* Waits for a child process pid and retrieves the child's exit status. */
// int wait (pid_t pid)
// {
// 	return NULL;
// }

// /* Creates a new file called file initially initial_size bytes in size. 
//    Returns true if successful, false otherwise. Creating a new file does
//    not open it: opening the new file is a separate operation which would 
//    require a open system call. */
// bool create (const char *file, unsigned initial_size)
// {
// 	return NULL;
// }

// /* Deletes the file called file. Returns true if successful, false otherwise.
//    A file may be removed regardless of whether it is open or closed, and removing 
//    an open file does not close it. See Removing an Open File, for details */
// bool remove (const char *file)
// {
// 	return NULL;
// }

