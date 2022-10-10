#ifndef CSCRIPT_H
#define CSCRIPT_H

int cslog(void *out, const char *s);
int cslogf(void *out, const char *s, ...);

typedef struct
{
  int init_code;
  char *iwd; //initial working dir

  //arena
  char *m;
  size_t size;
  size_t used;
} cscript_state;
extern cscript_state css;
void *arena_malloc(size_t s, cscript_state *css);
int cscript_init_size(size_t mb);
int cscript_report();
int cscript_cleanup(); //recommended to NOT do this! (leave to OS)

//config
struct tcscript_obj;
typedef char csobjt;
extern const csobjt OBJ_T;
extern const csobjt ARR_T;
extern const csobjt INT_T;
extern const csobjt FLOAT_T;
extern const csobjt STR_T;
extern const csobjt COMMENT_T;
struct tcscript_obj
{
  char *name;
  csobjt type;
  size_t n;
  union
  {
    struct tcscript_obj *objd;
    long long int intd;
    double floatd;
    char *strd;
  };
};
typedef struct tcscript_obj cscript_obj;
int cs_read_config(const char *file, cscript_obj **config);

/*
norms:
success cs_operation(in0, in1, ..., out0, out1, ...);
'get' = malloc
'grab' = wrapper for malloc; return malloced
*/

//util
int cs_strlen(const char *str); //(len not including null term)
int cs_strcmp(const char *a, const char *b); //(0 if equivalent up to null term)
int cs_strcpy(const char *from, char *to); //copy from -> to, including null term (len not including null term)
int cs_getstrcpy(const char *from, char **to); //copy from -> malloc to, including null term (len not including null term)
int cs_getstrcat(const char *a, const char *b, char **concat); //copy ab -> malloc concat, including null term (len not including null term)

int cs_getlinefromargs(int argc, const char **argv, char **args); //parse argv -> malloc args (len not including null term)
int cs_getargsfromline(const char *args, char ***argv); //parse args -> malloc^2 argv (argc)

int cs_getquote(const char *s, char **quote); //adds " to either end (len not including null term)
int cs_gettstr(long long unsigned int t, char **str); //YYYYMMDDHHMMSSmmm //17 chars
int cs_getswapstr(const char *base, const char *pattern, const char *replace, char **str); //swaps pattern for replace in base (len not including null term)
int cs_fileext(const char *file); //(position after .)
int cs_filepath(const char *file); //(length to filename)

//grab wrappers
char *cs_grabstrcpy(const char *from);
char *cs_grabstrcat(const char *a, const char *b);
char *cs_grablinefromargs(int argc, const char **argv);
char *cs_grabquote(const char *s);
char *cs_grabtstr(long long unsigned int t);
char *cs_grabswapstr(const char *base, const char *pattern, const char *replace);
//sugar
char *cat(const char* base, ...); //chained cs_grabstrcat; _final arg must be null_
char *dcat(const char *delim, const char* base, ...); //chained cs_grabstrcat with delimiter; _final arg must be null_

//env
int set_env(const char *name, const char *val);
int get_env(const char *name, char **val);
//sugar
char *grab_env(const char *name);

//fs
int create_file(const char *file, const char *contents);
int read_file(const char *file, char **contents);
int append_file(const char *file, const char *contents);
int move_file(const char *from, const char *to);
int copy_file(const char *from, const char *to);
int delete_file(const char *file);
int create_folder(const char *folder);
int move_folder(const char *from, const char *to);
int copy_folder(const char *from, const char *to);
int delete_folder(const char *folder);
int touch_filestructure(const char *structure);

int resolve_relative_path(const char *rpath, char **apath);
int get_cwd(char **folder);
int set_cwd(const char *folder);
int get_files_in_folder(const char *folder, char ***files);
int get_folders_in_folder(const char *folder, char ***folders);
int file_exists(const char *file);
int folder_exists(const char *folder);
long long unsigned int file_last_access(const char *file);
int file_depends(const char *file, const char **deps, int ndeps);
int file_depends_line(const char *file, const char *deps);
//sugar
char *grab_cwd();

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

