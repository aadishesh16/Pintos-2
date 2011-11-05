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
#include "devices/shutdown.h"

static void syscall_handler (struct intr_frame *);

static void copy_in(void* dest, void* src, int size);
void sys_halt(void);
void sys_exit(int status);
pid_t sys_exec(const char *cmd_line);
int sys_wait(pid_t pid);
bool sys_create(const char *file, unsigned initial_size);
bool sys_remove(const char *file);
int sys_open(const char *file);
int sys_filesize(int fd);
int sys_read(int fd, void *buffer, unsigned size);
int sys_write(int fd, const void *buffer, unsigned size);
void sys_seek(int fd, unsigned position);
unsigned sys_tell(int fd);
void sys_close(int fd);


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
  printf ("system call!\n");
  //thread_exit ();

  typedef int syscall_function (int, int, int);

  /* A system call. */
  struct syscall
  {
    size_t arg_cnt;     /* Number of arguments */
    syscall_function *func;   /*Implimentation*/
  };

  /* Table of system calls. */
  static const struct syscall syscall_table[] =
  {
    {0, (syscall_function *) sys_halt},
    {1, (syscall_function *) sys_exit},
    {1, (syscall_function *) sys_exec},
    {1, (syscall_function *) sys_wait},
    {2, (syscall_function *) sys_create},
    {1, (syscall_function *) sys_remove},
    {1, (syscall_function *) sys_open},
    {1, (syscall_function *) sys_filesize},
    {3, (syscall_function *) sys_read},
    {3, (syscall_function *) sys_write},
    {2, (syscall_function *) sys_seek},
    {1, (syscall_function *) sys_tell},
    {1, (syscall_function *) sys_close},
  };

  const struct syscall *sc;
  unsigned call_nr;
  int args[3];
  /* Get the system call. */
  copy_in (&call_nr, f->esp, sizeof call_nr);
  if (call_nr >= sizeof syscall_table / sizeof *syscall_table)
    thread_exit();
  sc = syscall_table + call_nr;
  printf("syscall = %d\n\n", call_nr);

  /* Get the system call arguments. */
  ASSERT (sc->arg_cnt <= sizeof args / sizeof *args);
  memset(args, 0, sizeof args);
  copy_in (args, (uint32_t *) f->esp + 1, sizeof * args * sc->arg_cnt);

  /* Execute the system call, and set the return value. */
  f->eax = sc->func (args[0], args[1], args[2]);
}

static void
copy_in(void* dest, void* src, int size)
{
  //ASSERT (dest != NULL || size == 0);
  //ASSERT (src != NULL || size == 0);

  for (; size > 0; size--, dest++, src++)
    *(char *)dest = *(char *)src;
}

/*Terminates Pintos by calling power_off() (declared in threads/init.h). 
 * This should be seldom used, because you lose some information about 
 * possible deadlock situations, etc.*/
void
sys_halt(void)
{
  shutdown_power_off();
}

/* Terminates the current user program, returning status to the kernel. 
 * If the process's parent waits for it (see below), this is the status 
 * that will be returned. Conventionally, a status of 0 indicates success 
 * and nonzero values indicate errors. */
void
sys_exit(int status)
{
  thread_exit();
}

/*Runs the executable whose name is given in cmd_line, passing any given arguments, 
 * and returns the new process's program id (pid). Must return pid -1, which otherwise 
 * should not be a valid pid, if the program cannot load or run for any reason. 
 * Thus, the parent process cannot return from the exec until it knows whether the 
 * child process successfully loaded its executable. You must use appropriate 
 * synchronization to ensure this.*/
pid_t
sys_exec(const char *cmd_line)
{
  printf("sys_exec\n");
  return -1;
}


/*Waits for a child process pid and retrieves the child's exit status. If pid is still 
 * alive, waits until it terminates. Then, returns the status that pid passed to exit. 
 * If pid did not call exit(), but was terminated by the kernel (e.g. killed due to an 
 * exception), wait(pid) must return -1. It is perfectly legal for a parent process to 
 * wait for child processes that have already terminated by the time the parent calls 
 * wait, but the kernel must still allow the parent to retrieve its child's exit status, 
 * or learn that the child was terminated by the kernel.
 *
 * Much more on the PDF......*/
int
sys_wait(pid_t pid)
{
  printf("sys_wait\n");
  int wait = process_wait(pid);
  return wait;
}


/*Creates a new file called file initially initial_size bytes in size. Returns true if 
 * successful, false otherwise. Creating a new file does not open it: opening the new file 
 * is a separate operation which would require a open system call.*/
bool
sys_create(const char *file, unsigned initial_size)
{
  bool win;
  win = filesys_create(file, initial_size);
  return win;
}

/*Deletes the file called file. Returns true if successful, false otherwise. A file may be 
 * removed regardless of whether it is open or closed, and removing an open file does not 
 * close it. See Removing an Open File, for details.*/
bool
sys_remove(const char *file)
{
  printf("sys_remove\n");
  return false;
}

/*Opens the file called file. Returns a nonnegative integer handle called a 
 * "file descriptor" (fd), or -1 if the file could not be opened.
 *
 * more in pdf....*/
int
sys_open(const char *ufile)
{
  char *kfile = copy_in_string (ufile);
  struct file_descriptor *fd;
  int handle = -1;

  fd = malloc (sizeof *fd);
  if (fd != NULL)
  {
    lock_acquire (&fs_lock);
    fd->file = filesys_open (kfile);
    if (fd->file != NULL)
    {
      struct thread *cur = thread_current ();
      handle = fd->handle = cur->next_handle++;
      list_push_front (&cur->fds, &fd->elem);
    }
    else
      free (fd);
    lock_release (&fs_lock);
  }

  palloc_free_page (kfile);
  return handle;
}

/*Returns the size, in bytes, of the file open as fd.*/
int
sys_filesize(int fd)
{
  printf("sys_filesize\n");
  return -1;
}

/*Reads size bytes from the file open as fd into buffer. Returns the number of bytes
 * actually read (0 at end of file), or -1 if the file could not be read (due to a 
 * condition other than end of file). Fd 0 reads from the keyboard using input_getc().*/
int
sys_read(int fd, void *buffer, unsigned size)
{
  printf("sys_read\n");
  return -1;
}

/*Writes size bytes from buffer to the open file fd. Returns the number of bytes actually
 * written, which may be less than size if some bytes could not be written.
 *
 * Writing past end-of-file would normally extend the file, but file growth is not 
 * implemented by the basic file system. The expected behavior is to write as many bytes as 
 * possible up to end-of-file and return the actual number written, or 0 if no bytes could be 
 * written at all. Fd 1 writes to the console. Your code to write to the console should write 
 * all of buffer in one call to putbuf(), at least as long as size is not bigger than a few 
 * hundred bytes. (It is reasonable to break up larger buffers.) Otherwise, lines of text output
 * by different processes may end up interleaved on the console, confusing both human readers 
 * and our grading scripts.*/
int
sys_write(int fd, const void *buffer, unsigned size)
{
  if (fd == 1){
    putbuf(buffer, size);
  }
  return 0;
}

/*Changes the next byte to be read or written in open file fd to position, expressed in bytes 
 * from the beginning of the file. (Thus, a position of 0 is the file's start.)
 *
 * A seek past the current end of a file is not an error. A later read obtains 0 bytes, 
 * indicating end of file. A later write extends the file, filling any unwritten gap with 
 * zeros. (However, in Pintos files have a fixed length until project 4 is complete, so 
 * writes past end of file will return an error.) These semantics are implemented in the 
 * file system and do not require any special effort in system call implementation.*/
void
sys_seek(int fd, unsigned position)
{
  printf("sys_seek\n");
}

/*Returns the position of the next byte to be read or written in open file fd, expressed 
 * in bytes from the beginning of the file.*/
unsigned
sys_tell(int fd)
{
  printf("sys_tell\n");
  return -1;
}

/*Closes file descriptor fd. Exiting or terminating a process implicitly closes all its 
 * open file descriptors, as if by calling this function for each one.*/
void
sys_close(int fd)
{
  printf("sys_close\n");
}
