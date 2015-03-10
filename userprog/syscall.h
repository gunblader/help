
#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include <list.h>
struct file_info{
	int fd;
	char *name;
    struct list_elem file_list_elem;
};

void syscall_init (void);

#endif /* userprog/syscall.h */
