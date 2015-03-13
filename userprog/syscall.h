
#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include "lib/kernel/list.h"

// struct file_info{
//   int fd;
//   char *name;
//   struct file *file;
//   struct list_elem file_list_elem;
// };

void syscall_init (void);

#endif /* userprog/syscall.h */
