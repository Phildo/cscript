#include "cscript.h"

#include <stdio.h>

int main(int argc, char **argv)
{
  cscript_init_size(2);
  char *cwd;
  get_cwd(&cwd);
  printf("cwd: %s\n",cwd);
  set_cwd("..");
  get_cwd(&cwd);
  printf("cwd: %s\n",cwd);

  cscript_cleanup();
}
