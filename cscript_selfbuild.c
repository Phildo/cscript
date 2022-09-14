#define CSCRIPT_IMPL
#include "cscript.h"

#include <stdio.h>

int main(int argc, char **argv)
{
  cscript_init_size(1);

  char *CC     = "C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Tools\\Llvm\\x64\\bin\\clang.exe";
  char *CCARGS = "-Wall";
  char *TEST_EXE  = "test.exe";
  char *BUILD_EXE = "selfbuild.exe";
  char *cfiles = "cscript_test.c cscript_selfbuild.c";
  char *ofiles = "cscript_test.o cscript_selfbuild.o";
  run_args(CC, dcat(" ",CCARGS,"-c",          cfiles,               0));
  run_args(CC, dcat(" ",CCARGS,"-o",TEST_EXE, "cscript_test.o",     0));
  //run_args(CC, dcat(" ",CCARGS,"-o",BUILD_EXE,"cscript_selfbuild.o",0));

  run(TEST_EXE);
  //run(BUILD_EXE);

  char **ofile_list;
  int ofile_n = cs_getargsfromline(ofiles,&ofile_list);
  for(int i = 0; i < ofile_n; i++)
    delete_file(ofile_list[i]);
  delete_file(TEST_EXE);
  delete_file(cs_grabswapstr(TEST_EXE,".exe",".pdb"));
  delete_file(cs_grabswapstr(TEST_EXE,".exe",".ilk"));

  //delete_file(BUILD_EXE);
  delete_file(cs_grabswapstr(BUILD_EXE,".exe",".pdb"));
  delete_file(cs_grabswapstr(BUILD_EXE,".exe",".ilk"));

  cscript_cleanup();
}
