#include <stdio.h>
#include <syscall.h>

int
main (int argc, char **argv)
{
  int i;

  bool lawl = create("newfile.txt", 1);

  if(lawl) printf("File created!!\n\n");

  bool rofl = remove("newfile.txt");

  if (rofl) printf("File deleted!!!\n\n");

  return EXIT_SUCCESS;
}
