#include "../cscript.h"

int main(int argc, const char **argv)
{
  cscript_init_size(1);
  
  const char *CC = "C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Tools\\Llvm\\x64\\bin\\clang.exe";

  run_args(CC, "build.c -o build.exe");
}
