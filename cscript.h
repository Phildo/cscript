#ifndef CSCRIPT_H
#define CSCRIPT_H

typedef struct
{
  char *m;
  size_t size;
  size_t used;
} cscript_arena;
void *arena_malloc(size_t s, cscript_arena *arena);

typedef struct
{
  char *iwd; //initial working dir
  cscript_arena arena;
} cscript_state;
extern cscript_state css;
int cscript_init_size(size_t mb);
int cscript_cleanup(); //recommended to NOT do this! (leave to OS)

/*
norms:
success cs_operation(in0, in1, ..., out0, out1, ...);
'get' = malloc
*/

//util
int cs_strlen(const char *str); //(len not including null term)
int cs_strcmp(const char *a, const char *b); //(0 if equivalent up to null term)
int cs_strcpy(const char *from, char *to); //copy from -> to, including null term (len not including null term)
int cs_getstrcpy(const char *from, char **to); //copy from -> malloc to, including null term (len not including null term)
int cs_getstrcat(const char *a, const char *b, char **concat); //copy ab -> malloc concat, including null term (len not including null term)

int cs_getlinefromargs(int argc, const char **argv, char **args); //parse argv -> malloc args (len not including null term)
int cs_getargsfromline(const char *args, char **argv); //parse args -> malloc^2 argv (argc)

int cs_gettstr(long long unsigned int t, char **str);
int cs_fileext(const char *file);
int cs_filepath(const char *file);

//fs
int create_file(const char *file, const char *contents);
int append_file(const char *file, const char *contents);
int move_file(const char *from, const char *to);
int copy_file(const char *from, const char *to);
int delete_file(const char *file);
int create_folder(const char *folder);
int move_folder(const char *from, const char *to);
int copy_folder(const char *from, const char *to);
int delete_folder(const char *folder);

int resolve_relative_path(const char *rpath, char **apath);
int get_cwd(char **folder);
int set_cwd(const char *folder);
int get_files_in_folder(const char *folder, char ***files);
int get_folders_in_folder(const char *folder, char ***folders);
long long unsigned int file_last_access(const char *file);
int file_depends(const char *file, const char **deps, int ndeps);

//run programs
int run(              const char *program);
int run_args(         const char *program, const char *args);
int run_stdin(        const char *program,                   void *sin);
int run_args_stdin(   const char *program, const char *args, void *sin);
int run_stdout(       const char *program,                              void **sout);
int run_args_stdout(  const char *program, const char *args,            void **sout);
int run_stdinout(     const char *program,                   void *sin, void **sout);
int run_args_stdinout(const char *program, const char *args, void *sin, void **sout);

#endif //CSCRIPT_H
