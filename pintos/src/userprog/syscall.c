#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/malloc.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "threads/init.h"
#include "threads/vaddr.h"
#include "threads/palloc.h"
#include <string.h>

static void syscall_handler (struct intr_frame *);

void
copy_in(char* dest, char* src, int size);


void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

/* Copies a byte from user addresses USRC to kernel address DST. */
// not working correctly here.....
static inline bool
get_user(uint8_t * dst, const uint8_t *usrc)
{
  int eax;
  asm ("movl $1f, %%eax; movb %2, %%al; movb %%al, %0; 1:"
        : "=m" (*dst), "=%a" (eax) : "m" (*usrc));
  return eax != 0;
}

/* Writes BYTE to user address UDST. */
static inline bool
put_user (uint8_t *udst, uint8_t byte)
{
  int eax;
  asm ("movl $1f, %%eax; movb %b2, %0; 1:"
        : "=n" (*udst), "=&a" (eax) : "q" (byte));
  return eax != 0;
}

static char *
copy_in_string (const char *us)
{
  char *ks;
  size_t length;
  ks = palloc_get_page (0);
  if (ks == NULL)
    thread_exit ();
  for (length = 0; length < PGSIZE; length++)
  {
      if (us >= (char *) PHYS_BASE || !get_user (ks + length, us++)) //problem with get_user
      {
        palloc_free_page (ks);
        thread_exit ();
      }
      if (ks[length] == '\0')
        return ks;
  }
  ks[PGSIZE -1] = '\0';
  return ks;
}

static void
syscall_handler (struct intr_frame *f) 
{
  //printf ("system call!\n");
  //thread_exit ();

  typedef int syscall_function (int, int, int);

  /* A system call. */
  struct syscall
  {
    size_t arg_cnt;     /* Number of arguments */
    syscall_function *func;   /*Implimentation*/
  }

  /* Table of system calls. */
  // Not done....?
  // warning 'static' is not at beginning of declaration
  // error: two or more data types in declaration specs
  // This is the code he gave us......
  static const struct syscall syscall_table[] =
  {
    {0, (syscall_function *) SYS_HALT},
    {1, (syscall_function *) SYS_EXIT},
    {1, (syscall_function *) SYS_EXEC},
    {1, (syscall_function *) SYS_WAIT},
    {2, (syscall_function *) SYS_CREATE},
    {1, (syscall_function *) SYS_REMOVE},
    {1, (syscall_function *) SYS_OPEN}
  };

  const struct syscall *sc;
  unsigned call_nr;
  int args[3];
  /* Get the system call. */
  // I think we have to write a copy_in function
  copy_in (&call_nr, f->esp, sizeof call_nr);
  if (call_nr >= sizeof syscall_table / sizeof *syscall_table)
    thread_exit();
  sc = syscall_table + call_nr;

  /* Get the system call arguments. */
  ASSERT (sc->arg_cnt <= sizeof args / sizeof *args);
  memset(args, 0, sizeof args);
  copy_in (args, (uint32_t *) f->esp + 1, sizeof * args * sc->arg_cnt);

  /* Execute the system call, and set the return value. */
  f->eax = sc->func (args[0], args[1], args[2]);
}

void
copy_in(char* dest, char* src, int size)
{
  unsigned char *dst = dest;
  const unsigned char *src_ = src;

  ASSERT (dst != NULL || size == 0);
  ASSERT (src_ != NULL || size == 0);

  while (size-- > 0)
    *(char *)dst++ = *(char *)src_++;
}

/*int
sys_open(const char *ufile)
{
  char *kfile = copy_in_string (ufile);
  struct file_descriptor *fd;
  int handle = -1;

  fd = malloc (sizeof *fd);
  if (fd != NULL)
  {
    //lock_acquire (&fs_lock); // not defined....?
    fd->file = filesys_open (kfile);
    if (fd->file != NULL) // error dereferencing pointer to incomplete type
    {
      // more errors here....
      struct thread *cur = thread_current ();
      handle = fd->handle = cur->next_handle++;
      list_push_front (&cur->fds, &fd->elem);
    }
    else
      free (fd);
    //lock_release (&fs_lock);
  }

  palloc_free_page (kfile);
  return handle;
}*/
