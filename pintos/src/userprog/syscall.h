#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include <list.h>

void syscall_init (void);

int
sys_open(const char *ufile);

/*struct file_descriptor
{
  int handle;
  //string file?
  
  struct listelem elem;
};*/

#endif /* userprog/syscall.h */
