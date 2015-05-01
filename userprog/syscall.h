
#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include "lib/kernel/list.h"
#include "filesys/directory.h"

struct file_info{
  int fd;
  char *name;
  struct file *file;
  struct list_elem file_list_elem;
  struct list_elem thread_file_list_elem;
};

struct dir;
struct inode;

void syscall_init (void);
bool end_parse(char *path, struct inode **parent_inode);

#endif /* userprog/syscall.h */
