#ifndef _WIN32
#error attempting to compile cscript for windows
#endif

#include "../cscript.h"

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

int cslog(void *out, const char *str)
{
  fprintf(out,"%s\n",str);
  fflush(out);
  return 0;
}

int cslogf(void *out, const char *str, ...)
{
  va_list args;
  va_start(args, str);

  vfprintf(out,str,args);
  fprintf(out,"\n");
  fflush(out);

  va_end(args);
  return 0;
}

#define CSCRIPT_INIT_CODE 123
cscript_state css;

void *arena_malloc(size_t s, cscript_state *css)
{
  assert(css->init_code == CSCRIPT_INIT_CODE);
  assert(css->m && css->size);
  if(s%4) s = (s/4+1)*4; //always align 4
  if(css->used+s > css->size)
  {
    size_t new_s = css->size*2;
    size_t new_s_mb = new_s/1024/1024;
    cslogf(stderr, "REALLOCING ARENA- RECOMMENDED TO INIT TO AT LEAST %lluMB FOR FUTURE BUILDS",new_s_mb);
    void *re = realloc(css->m, css->size*2);
    if(!re)
    {
      cslogf(stderr, "FAILED TO REALLOC ARENA- INIT TO AT LEAST %lluMB FOR FUTURE BUILDS",new_s_mb);
      exit(1);
    }
    if(re != css->m)
    {
      cslogf(stderr, "REALLOCED ARENA MOVED; CERTAIN TO RESULT IN OUTDATED POINTERS; INIT TO AT LEAST %lluMB AND RE-BUILD",new_s_mb);
      exit(1);
      css->m = re;
    }
    css->size = new_s;
  }
  void *r = css->m+css->used;
  css->used += s;
  return r;
}

int cscript_init_size(size_t mb)
{
  css.size = 1024*1024*mb;
  css.used = 0;
  css.m = (char *)malloc(css.size);
  css.init_code = CSCRIPT_INIT_CODE;
  css.iwd = grab_cwd();

  return css.size;
}

int cscript_report()
{
  if(css.init_code != CSCRIPT_INIT_CODE)
  {
    printf("cscript NOT INIT- run cs_init_size(MB) before using\n");
    return 0;
  }
  assert(css.init_code == CSCRIPT_INIT_CODE);
  printf("arena memory used %llu (%0.1fMB) of %llu (%0.1fMB) %llu%%\n",css.used,css.used/1024.0f/1024.0f,css.size,css.size/1024.0f/1024.0f,css.used*100/css.size);
  return 1;
}

int cscript_cleanup()
{
  assert(css.init_code == CSCRIPT_INIT_CODE);
  printf("cleaning cscript- NOT RECOMMENDED; end program and let OS clean up!\n");
  cscript_report();
  free(css.m);
  css.m = NULL;
  css.init_code = 0;
  return 0;
}

//config
static int read_character(const char *contents, int size, char c)
{
  int i = 0;
  if(contents[i] == c) i++;
  else return -1;
  return i;
}
static int read_whitespace(const char *contents, int size)
{
  int i = 0;
  while(i < size)
  {
    switch(contents[i])
    {
      case ' ':
      case '\t':
      case '\n':
      case '\r':
        i++;
        break;
      default:
        return i;
    }
  }
  return i;
}
static int read_comment(const char *contents, int size)
{
  int i = 0;
  int ei;

  ei = read_character(contents+i,size-i,'/');
  if(ei == -1) return -1;
  else i += ei;
  ei = read_character(contents+i,size-i,'/');
  if(ei == -1) return -1;
  else i += ei;

  while(i < size)
  {
    switch(contents[i])
    {
      case '\n':
        return i;
      default:
        i++;
    }
  }
  return i;
}
static int read_null(const char *contents, int size)
{
  int i = 0;
  int ei;
  int interesting = 1;
  while(interesting)
  {
    i += read_whitespace(contents+i,size-i);
    ei = read_comment(contents+i,size-i);
    if(ei == -1) interesting = 0;
    else i += ei;
  }
  return i;
}
static int read_string(const char *contents, int size, char **string)
{
  int i = 0;
  int ei;
  ei = read_character(contents+i,size-i,'"');
  if(ei == -1) return -1;
  else i += ei;
  int j = i;
  while(j < size && contents[j] != '"') j++;
  if(j == size) return -1;
  *string = arena_malloc(j-i,&css);
  j = 0;
  while(contents[i] != '"') { (*string)[j] = contents[i]; j++; }
  i++; //ei = read_character(contents+i,size-i,'"'); //guaranteed
  return i;
}
static int read_name(const char *contents, int size, char **name)
{
  int i = 0;
  int ei;
  i += read_null(contents+i,size-i);
  ei = read_string(contents+i,size-i,name);
  if(ei == -1)
  {
    cslogf(stderr, "failed to read config file name");
    exit(1);
  }
  else i += ei;
  i += read_null(contents+i,size-i);
  i += read_character(contents+i,size-i,':');
  return i;
}
static int read_data(const char *contents, int size, cscript_obj *obj)
{
  int i = 0;
  int ei;
  i += read_null(contents+i,size-i);
  while(i < size)
  {
    switch(contents[i++])
    {
      case '{':
      case '[':
      {
        char close;

        if(contents[i-1] == '{')
        {
          close = '}';
          obj->type = OBJ_T;
        }
        else if(contents[i-1] == '[')
        {
          obj->type = ARR_T;
          close = ']';
        }
        else assert(0);

        obj->n = 0;
        size_t alloced = 4;
        cscript_obj *tobjs = arena_malloc(alloced*sizeof(cscript_obj),&css);
        int interesting = 1;
        while(interesting)
        {
          ei = read_character(contents+i,size-i,close);
          if(ei == -1)
          {
            if(obj->type == OBJ_T)
            {
              i += read_null(contents+i,size-i);
              i += read_name(contents+i,size-i,&obj->name);
            }
            else obj->name = NULL;
            i += read_null(contents+i,size-i);
            if(obj->n == alloced)
            {
              cscript_obj *ttobjs = arena_malloc(alloced*2*sizeof(cscript_obj),&css);
              memcpy(ttobjs,tobjs,alloced*(sizeof(cscript_obj)));
              tobjs = ttobjs;
              alloced *= 2;
            }
            i += read_data(contents+i,size-i,&tobjs[obj->n]);
            obj->n++;
            i += read_null(contents+i,size-i);
            ei = read_character(contents+i,size-i,',');
            if(ei == -1) interesting = 0;
            else i += ei;
          }
          else interesting = 0;
        }
        ei = read_character(contents+i,size-i,close);
        if(ei == -1)
        {
          cslogf(stderr, "failed to read obj object data");
          exit(1);
        }
        obj->objd = tobjs;
      }
      //']'; //garbage for matching []
      //'}'; //garbage for matching {}
        break;
      case '"':
      {
        obj->type = STR_T;
        obj->n = 1;
        ei = read_string(contents+i,size-i,&obj->strd);
        if(ei == -1)
        {
          cslogf(stderr, "failed to read obj string data");
          exit(1);
        }
        else i += ei;
      }
        break;
      case '-':
      case '.':
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
      {
        obj->type = INT_T;
        obj->n = 1;
        int sign = 1;
        long long int intd = 0;
        double floatd = 0.0;
        double mantissaplace;
        ei = read_character(contents+i,size-i,'-');
        if(ei != -1) { i++; sign = -1; }
        while(i < size && contents[i] >= '0' && contents[i] <= '9')
        {
          intd *= 10;
          intd += contents[i++]-'0';
        }
        if(contents[i] == '.')
        {
          i++;
          obj->type = FLOAT_T;
          floatd = intd;
          mantissaplace = 0.1;
          while(i < size && contents[i] >= '0' && contents[i] <= '9')
          {
            floatd += (contents[i++]-'0')*mantissaplace;
            mantissaplace *= 0.1;
          }
          obj->floatd = sign*floatd;
        }
        else obj->intd = sign*intd;
      }
        break;
      default:
      {
        cslogf(stderr, "failed to read obj data");
        exit(1);
      }
    }
  }
  return i;
}
int cs_read_config(const char *file, cscript_obj **config)
{
  char *contents;
  int size = read_file(file, &contents);
  *config = arena_malloc(sizeof(cscript_obj),&css);
  (*config)->name = "root";

  int i = 0;
  int ei;
  i += read_null(contents+i,size-i);
  ei = read_data(contents+i,size-i,*config);
  if(ei == -1)
  {
    cslogf(stderr, "failed to read config file data: %s",file);
    exit(1);
  }
  else i += ei;

  i += read_null(contents+i,size-i);
  assert(i == size);
  return i;
}
cscript_obj *cs_objgetnocpy(char *name, cscript_obj *obj)
{
  size_t i = 0;
  while(1)
  {
    switch(name[i])
    {
      case '\0':
      case '.':
      case '[':
      {
        if(i > 0)
        {
          if(obj->type != OBJ_T)
          {
            cslogf(stderr, "looking for name in non-object",name);
            exit(1);
          }
          char replace = name[i];
          name[i] = '\0';
          for(int j = 0; j < obj->n; j++)
          {
            if(cs_strcmp(name,obj->objd[j].name) == 0)
            {
              name[i] = replace;
              switch(replace)
              {
                case '\0': return &obj->objd[j]; break;
                case '.': return cs_objgetnocpy(name+i+1, &obj->objd[j]); break;
                case '[': return cs_objgetnocpy(name+i, &obj->objd[j]); break;
              }
            }
          }
        }
        else //i == 0
        {
          if(name[i] == '[')
          {
            if(obj->type != ARR_T)
            {
              cslogf(stderr, "looking for index in non-array",name);
              exit(1);
            }
            i++;
            int index = 0;
            while(name[i] == ' ' || name[i] == '\t')
              i++;
            while(name[i] >= '0' && name[i] <= '9')
            {
              index *= 10;
              index += name[i]-'0';
              i++;
            }
            while(name[i] == ' ' || name[i] == '\t')
              i++;
            if(name[i] != ']')
            {
              cslogf(stderr, "failed to find index in path",name);
              exit(1);
            }
            i++;
            if(index > obj->n)
            {
              cslogf(stderr, "out of bound index",name);
              exit(1);
            }
            if(name[i] == '\0') return &obj->objd[index];
            else return cs_objgetnocpy(name+i, &obj->objd[index]);
          }
          else
          {
            cslogf(stderr, "failed to find path in obj %s",name);
            exit(1);
          }
        }
      }
        break;
      default: i++;
    }
  }
}
cscript_obj *cs_objget(const char *path, cscript_obj *obj)
{
  return cs_objgetnocpy(cs_grabstrcpy(path),obj);
}
int cs_write_config(const char *file, cscript_obj **config)
{
  return -1;
}

//util
int cs_strlen(const char *str)
{
  int i = 0;
  while(str[i] != '\0') i++;
  return i;
}
int cs_strcmp(const char *a, const char *b)
{
  int i = 0;
  while(a[i] != '\0' && a[i]-b[i] != 0)
    i++;
  return a[i]-b[i];
}
int cs_strcpy(const char *from, char *to)
{
  int i = 0;
  while(from[i] != '\0')
  {
    to[i] = from[i];
    i++;
  }
  to[i] = '\0';
  return i;
}
int cs_getstrcpy(const char *from, char **to)
{
  int i = cs_strlen(from);
  *to = arena_malloc(i+1,&css);
  cs_strcpy(from,*to);
  return i;
}
int cs_getstrcat(const char *a, const char *b, char **concat)
{
  int alen = cs_strlen(a);
  int blen = cs_strlen(b);
  *concat = arena_malloc(alen+blen+1,&css);
  cs_strcpy(a,*concat);
  cs_strcpy(b,*concat+alen);
  return alen+blen;
}
int cs_getlinefromargs(int argc, const char **argv, char **args)
{
  int len = 0;
  for(int i = 0; i < argc; i++)
    len += (cs_strlen(argv[i])+1);
  *args = arena_malloc(len,&css);
  len = 0;
  for(int i = 0; i < argc; i++)
  {
    len += cs_strcpy(argv[i],(*args)+len);
    *((*args)+len) = ' ';
    len++;
  }
  *((*args)+len) = '\0';
  len--;
  return len;
}
int cs_getargsfromline(const char *args, char ***argv)
{
  *argv = arena_malloc(sizeof(char **)*10,&css);
  char *argcpy = cs_grabstrcpy(args);
  int argc = 0;
  int len = 0;
  int strlen = 0;
  while(1)
  {
    switch(argcpy[len])
    {
      case ' ':
      case '\t':
      case '\n':
      case '\r':
        argcpy[len] = '\0';
        if(!strlen) break;
        strlen = 0;
        argc++;
        break;
      case '\0':
        if(strlen) argc++;
        return argc;
        break;
      default:
        if(!strlen) (*argv)[argc] = &argcpy[len];
        strlen++;
        break;
    }
    len++;
  }
}
int cs_getquote(const char *s, char **quote)
{
  int i = cs_strlen(s);
  *quote = arena_malloc(i+3,&css);
  *(*quote) = '"';
  cs_strcpy(s,(*quote)+1);
  *((*quote)+1+i)   = '"';
  *((*quote)+1+i+1) = '\0';
  return i;
}
int cs_gettstr(long long unsigned int t, char **str)
{
  FILETIME ft;
  ft.dwLowDateTime = t & 0xFFFFFFFF;
  t >>= 32;
  ft.dwHighDateTime = t;

  SYSTEMTIME stUTC, stLocal;
  FileTimeToSystemTime(&ft, &stUTC);
  SystemTimeToTzSpecificLocalTime(NULL, &stUTC, &stLocal);

  //YYYYMMDDHHMMSSmmm //17 chars
  *str = arena_malloc(18,&css);
  sprintf_s(*str, 18, "%04d%02d%02d%02d%02d%02d%03d", stLocal.wYear, stLocal.wMonth, stLocal.wDay, stLocal.wHour, stLocal.wMinute, stLocal.wSecond, stLocal.wMilliseconds);
  return 17;
}
int cs_getswapstr(const char *base, const char *pattern, const char *replace, char **str)
{
  int baselen = cs_strlen(base);
  int patternlen = cs_strlen(pattern);
  int replacelen = cs_strlen(replace);

  int i = 0;
  int nmatch = 0;
  while(base[i] != '\0')
  {
    if(base[i] == pattern[0])
    {
      int matchi = 1;
      while(base[i+matchi] == pattern[matchi] && base[i+matchi] != '\0' && pattern[matchi] != '\0')
        matchi++;
      if(pattern[matchi] == '\0')
      {
        assert(matchi == patternlen);
        nmatch++;
        i += matchi-1;
      }
    }
    i++;
  }

  *str = arena_malloc(baselen + nmatch*(replacelen-patternlen) + 1, &css);

  int readi = 0;
  int writei = 0;
  while(base[readi] != '\0')
  {
    if(base[readi] == pattern[0])
    {
      int matchi = 1;
      while(base[readi+matchi] == pattern[matchi] && base[readi+matchi] != '\0' && pattern[matchi] != '\0')
        matchi++;
      if(pattern[matchi] == '\0')
      {
        assert(matchi == patternlen);
        for(int i = 0; i < replacelen; i++)
          (*str)[writei++] = replace[i];
        readi += matchi-1;
      }
      else (*str)[writei++] = base[readi];
    }
    else (*str)[writei++] = base[readi];
    readi++;
  }
  (*str)[writei] = '\0';

  return writei;
}

//grab wrappers
char *cs_grabstrcpy(const char *from)                  { char *r; cs_getstrcpy(from, &r);             return r; }
char *cs_grabstrcat(const char *a, const char *b)      { char *r; cs_getstrcat(a, b, &r);             return r; }
char *cs_grablinefromargs(int argc, const char **argv) { char *r; cs_getlinefromargs(argc, argv, &r); return r; }
char *cs_grabquote(const char *s)                      { char *r; cs_getquote(s, &r);                 return r; }
char *cs_grabtstr(long long unsigned int t)            { char *r; cs_gettstr(t, &r);                  return r; }
char *cs_grabswapstr(const char *base, const char *pattern, const char *replace) { char *r; cs_getswapstr(base,pattern,replace,&r); return r; }
//sugar
char *cat(const char* base, ...)
{
  char *r = (char *)base;
  va_list args;
  va_start(args, base);

  const char *a = va_arg(args, const char *);
  while(a != NULL)
  {
    r = cs_grabstrcat(r,a);
    a = va_arg(args, const char *);
  }

  va_end(args);
  return r;
}
char *dcat(const char *delim, const char* base, ...)
{
  char *r = (char *)base;
  va_list args;
  va_start(args, base);

  const char *a = va_arg(args, const char *);
  while(a != NULL)
  {
    r = cs_grabstrcat(r,delim);
    r = cs_grabstrcat(r,a);
    a = va_arg(args, const char *);
  }

  va_end(args);
  return r;
}

//env
int set_env(const char *name, const char *val)
{
  SetEnvironmentVariable(name, val);
  return 1;
}

int get_env(const char *name, char **val)
{
  int size = GetEnvironmentVariable(name,NULL,0);
  if(!size) return 0;
  *val = arena_malloc(size,&css);
  GetEnvironmentVariable(name,*val,size);
  return 1;
}

//sugar
char *grab_env(const char *name) { char *r; get_env(name,&r); return r; }

//fs
int create_file(const char *file, const char *contents)
{
  cslogf(stdout, "creating file %s",file);
  FILE *f;
  if(fopen_s(&f,file,"w"))
  {
    cslogf(stderr, "failed to open file for writing %s",file);
    exit(1);
  }
  int r = fputs(contents, f);
  if(r == EOF)
  {
    cslogf(stderr, "error writing string to file %s",file,contents);
    exit(1);
  }
  fclose(f);
  return r;
}
int read_file(const char *file, char **contents)
{
  cslogf(stdout, "reading file %s",file);
  FILE *f;
  if(fopen_s(&f,file,"r"))
  {
    cslogf(stderr, "failed to open file for reading %s",file);
    exit(1);
  }
  fseek(f, 0, SEEK_END);
  int size = ftell(f);
  fseek(f, 0, SEEK_SET);
  if(size == 0xFFFFFFFF)
  {
    cslogf(stderr, "failed to read file %s; file too large",file);
    exit(1);
  }
  if(size > 1024*1024 || size > css.size-css.used) //file large enough to use own malloc buffer
    *contents = malloc(size);
  else
    *contents = arena_malloc(size,&css);
  fgets(*contents,size,f);
  fclose(f);
  return size;
}
int append_file(const char *file, const char *contents)
{
  cslogf(stdout, "appending file %s",file);
  FILE *f;
  if(fopen_s(&f,file,"a"))
  {
    cslogf(stderr, "failed to open file for appending %s",file);
    exit(1);
  }
  int r = fputs(contents, f);
  if(r == EOF)
  {
    cslogf(stderr, "error appending string to file %s\n%s",file,contents);
    exit(1);
  }
  fclose(f);
  return r;
}
int move_file(const char *from, const char *to)
{
  cslogf(stdout, "moving file %s to %s",from,to);
  if(MoveFile(from,to)) return 1;
  else
  {
    cslogf(stderr, "error moving file %s to %s",from,to);
    exit(1);
  }
  return 0;
}
int copy_file(const char *from, const char *to)
{
  cslogf(stdout, "copying file %s to %s",from,to);
  if(CopyFile(from,to,FALSE)) return 1;
  else
  {
    cslogf(stderr, "error copying file %s to %s",from,to);
    exit(1);
  }
  return 0;
}
int delete_file(const char *file)
{
  cslogf(stdout, "deleting file %s",file);
  if(DeleteFile(file)) return 1;
  else cslogf(stderr, "error deleting file %s",file);
  return 0;
}
int create_folder(const char *folder)
{
  cslogf(stdout, "creating folder %s",folder);
  if(CreateDirectory(folder,NULL)) return 1;
  else
  {
    switch(GetLastError())
    {
      case ERROR_ALREADY_EXISTS: cslogf(stderr, "error creating folder %s\nfolder already exists",folder); break;
      case ERROR_PATH_NOT_FOUND: cslogf(stderr, "error creating folder %s\none or more intermediate directories do not exist",folder); break;
    }
    exit(1);
  }
  return 0;
}
int move_folder(const char *from, const char *to)
{
  cslogf(stdout, "moving folder %s to %s",from,to);
  if(MoveFileEx(from, to, MOVEFILE_WRITE_THROUGH)) return 1;
  else
  {
    cslogf(stderr, "error moving folder %s to %s",from,to);
    exit(1);
  }
  return 0;
}
int copy_folder(const char *from, const char *to)
{
  cslogf(stdout, "copying folder %s to %s",from,to);
  create_folder(to);
  char *frombase = cs_grabstrcat(from,"\\");
  char *tobase   = cs_grabstrcat(to,  "\\");

  WIN32_FIND_DATA fdata; 
  HANDLE find = NULL; 
  char *filemask = cs_grabstrcat(frombase,"*.*");
  if((find = FindFirstFile(filemask, &fdata)) == INVALID_HANDLE_VALUE) 
  { 
    cslogf(stderr,"error copying folder %s to %s",from,to); 
    exit(1);
  } 

  char *fromfile;
  char *tofile;
  do
  { 
         if(cs_strcmp(fdata.cFileName, "." ) == 0) ;
    else if(cs_strcmp(fdata.cFileName, "..") == 0) ;
    else
    { 
      fromfile = cs_grabstrcat(frombase,fdata.cFileName);
      tofile   = cs_grabstrcat(tobase,fdata.cFileName);

      if(fdata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) 
        copy_folder(fromfile,tofile);
      else
        copy_file(fromfile,tofile);
    }
  } 
  while(FindNextFile(find, &fdata));

  FindClose(find);
  return 1;
}
int delete_folder(const char *folder)
{
  cslogf(stdout, "deleting folder %s",folder);
  char *folderbase = cs_grabstrcat(folder,"\\");

  WIN32_FIND_DATA fdata; 
  HANDLE find = NULL; 
  char *filemask = cs_grabstrcat(folderbase,"*.*");
  if((find = FindFirstFile(filemask, &fdata)) == INVALID_HANDLE_VALUE) 
  { 
    cslogf(stderr,"error deleting folder %s",folder); 
    exit(1);
  } 

  char *file;
  do
  { 
         if(cs_strcmp(fdata.cFileName, "." ) == 0) ;
    else if(cs_strcmp(fdata.cFileName, "..") == 0) ;
    else
    { 
      file = cs_grabstrcat(folderbase,fdata.cFileName);

      if(fdata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) 
        delete_folder(file);
      else
        delete_file(file);
    }
  } 
  while(FindNextFile(find, &fdata));

  FindClose(find);
  
  if(RemoveDirectory(folder)) return 1;
  else
  {
    cslogf(stderr,"error deleting folder %s",folder); 
    exit(1);
  }
  return 0;
}

int resolve_relative_path(const char *rpath, char **apath)
{
  return 0;
}
int get_cwd(char **folder)
{
  size_t needed_cwd_len = GetCurrentDirectory(0,NULL);
  if(!needed_cwd_len)
  {
    cslog(stderr, "failed to get current working directory");
    exit(1);
  }
  *folder = arena_malloc(needed_cwd_len,&css);
  if(GetCurrentDirectory(needed_cwd_len,*folder) != needed_cwd_len-1)
  {
    cslog(stderr, "inconsistent working directory length");
    exit(1);
  }
  return needed_cwd_len-1;
}
int set_cwd(const char *folder)
{
  if(SetCurrentDirectory(folder)) return 1;
  return 0;
}
int get_files_in_folder(const char *folder, char ***files)
{
  char *folderbase = cs_grabstrcat(folder,"\\");

  WIN32_FIND_DATA fdata; 
  HANDLE find = NULL; 
  char *filemask = cs_grabstrcat(folderbase,"*.*");
  if((find = FindFirstFile(filemask, &fdata)) == INVALID_HANDLE_VALUE) 
  { 
    cslogf(stderr,"error getting files in folder %s",folder); 
    exit(1);
  } 

  size_t max = 32;
  *files = arena_malloc(sizeof(char *)*max, &css);
  size_t n = 0;
  do
  { 
         if(cs_strcmp(fdata.cFileName, "." ) == 0) ;
    else if(cs_strcmp(fdata.cFileName, "..") == 0) ;
    else
    { 

      if(fdata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) 
        ;
      else
      {
        if(n == max)
        {
          max *= 2;
          char **newfiles = arena_malloc(sizeof(char *)*max, &css);
          for(int i = 0; i < n; i++)
            newfiles[i] = (*files)[i];
          *files = newfiles;
        }
        (*files)[n] = cs_grabstrcat(folderbase,fdata.cFileName);
        n++;
      }
    }
  } 
  while(FindNextFile(find, &fdata));

  FindClose(find);

  return n;
}
int get_folders_in_folder(const char *folder, char ***folders)
{
  char *folderbase = cs_grabstrcat(folder,"\\");

  WIN32_FIND_DATA fdata; 
  HANDLE find = NULL; 
  char *filemask = cs_grabstrcat(folderbase,"*.*");
  if((find = FindFirstFile(filemask, &fdata)) == INVALID_HANDLE_VALUE) 
  { 
    cslogf(stderr,"error getting folders in folder %s",folder); 
    exit(1);
  } 

  size_t max = 32;
  *folders = arena_malloc(sizeof(char *)*max, &css);
  size_t n = 0;
  do
  { 
         if(cs_strcmp(fdata.cFileName, "." ) == 0) ;
    else if(cs_strcmp(fdata.cFileName, "..") == 0) ;
    else
    { 

      if(fdata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) 
        ;
      else
      {
        if(n == max)
        {
          max *= 2;
          char **newfolders = arena_malloc(sizeof(char *)*max, &css);
          for(int i = 0; i < n; i++)
            newfolders[i] = (*folders)[i];
          *folders = newfolders;
        }
        (*folders)[n] = cs_grabstrcat(folderbase,fdata.cFileName);
        n++;
      }
    }
  } 
  while(FindNextFile(find, &fdata));

  FindClose(find);

  return n;
}
int file_exists(const char *file)
{
  HANDLE f;

  f = CreateFile(file, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
  if(f == INVALID_HANDLE_VALUE) return 0;
  DWORD attrib = GetFileAttributes(f);
  CloseHandle(f);    
  return (attrib & FILE_ATTRIBUTE_DIRECTORY) == 0;
}
int folder_exists(const char *folder)
{
  HANDLE f;

  f = CreateFile(folder, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
  if(f == INVALID_HANDLE_VALUE)
    return 0;
  DWORD attrib = GetFileAttributes(f);
  CloseHandle(f);    
  return (attrib & FILE_ATTRIBUTE_DIRECTORY) != 0;
}
long long unsigned int file_last_access(const char *file)
{
  long long unsigned int t;
  HANDLE f;

  f = CreateFile(file, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
  if(f == INVALID_HANDLE_VALUE)
    return 0xFFFFFFFFFFFFFFFF;

  FILETIME ft;
  if(!GetFileTime(f, NULL, NULL, &ft))
  {
    cslogf(stderr,"failed to get access time for file %s",file);
    exit(1);
    return 0xFFFFFFFFFFFFFFFF;
  }
  CloseHandle(f);    

  t = ft.dwHighDateTime;
  t <<= 32;
  t |= ft.dwLowDateTime;
  return t;
}
int file_depends(const char *file, const char **deps, int ndeps)
{
  long long unsigned int filet = file_last_access(file);
  if(!filet) return 1;

  for(int i = 0; i < ndeps; i++)
  {
    long long unsigned int dept = file_last_access(deps[i]);
    if(filet < dept) return 1;
  }
  return 0;
}
int file_depends_line(const char *file, const char *deps)
{
  char **deps_args;
  int ndeps = cs_getargsfromline(deps, &deps_args);
  return file_depends(file, (const char **)deps_args, ndeps);
}
//sugar
char *grab_cwd() { char *r; get_cwd(&r); return r; }

int run_args_stdinout(const char *program, const char *args, void *sin, void **sout)
{
  if(args) printf("%s %s\n",program,args);
  else printf("%s\n",program);

  char *qprogram = cs_grabquote(program);

  SECURITY_ATTRIBUTES sa;
  memset(&sa,0,sizeof(sa));
  sa.nLength = sizeof(sa);
  sa.bInheritHandle = TRUE;
  sa.lpSecurityDescriptor = NULL;

  HANDLE rserr;
  HANDLE wserr;
  HANDLE rsout;
  HANDLE wsout;
  if(sout)
  {
    if(!CreatePipe(&rserr, &wserr, &sa, 0)) return HRESULT_FROM_WIN32(GetLastError());
    if(!SetHandleInformation(rserr, HANDLE_FLAG_INHERIT, 0)) return HRESULT_FROM_WIN32(GetLastError());

    if(!CreatePipe(&rsout, &wsout, &sa, 0)) return HRESULT_FROM_WIN32(GetLastError());
    if(!SetHandleInformation(rsout, HANDLE_FLAG_INHERIT, 0)) return HRESULT_FROM_WIN32(GetLastError());
  }
  else
  {
    wserr = GetStdHandle(STD_ERROR_HANDLE);
    wsout = GetStdHandle(STD_OUTPUT_HANDLE);
  }


  HANDLE rsin;
  HANDLE wsin;
  if(sin)
  {
    if(!CreatePipe(&rsin, &wsin, &sa, 0)) return HRESULT_FROM_WIN32(GetLastError());
    if(!SetHandleInformation(wsin, HANDLE_FLAG_INHERIT, 0)) return HRESULT_FROM_WIN32(GetLastError());
  }
  else
  {
    rsin = GetStdHandle(STD_INPUT_HANDLE);
  }

  PROCESS_INFORMATION pi;
  memset(&pi,0,sizeof(pi));
 
  STARTUPINFO si;     
  memset(&si,0,sizeof(si));
  si.cb = sizeof(si);
  si.hStdError = wserr;
  si.hStdOutput = wsout;
  si.hStdInput = rsin;
  si.dwFlags |= STARTF_USESTDHANDLES;

  char *cargs;
  if(args) cargs = dcat(" ",qprogram,args,0);
  else     cargs = qprogram;

  if(!CreateProcess(
    program, // path
    cargs,   // command line
    NULL,    // process security attributes
    NULL,    // primary thread security attributes
    TRUE,    // handle inheritance
    0,       // creation flags
    NULL,    // use parent's environment
    NULL,    // use parent's cwd
    &si,     // STARTUPINFO
    &pi      // PROCESS_INFORMATION
  ))
  {
    cslogf(stderr,"error (%lu) running %s",GetLastError(),cargs);
    exit(1);
  }

  if(sout)
  {
    CloseHandle(wserr);
    CloseHandle(wsout);
  }
  if(sin)
  {
    CloseHandle(rsin);
  }

  WaitForSingleObject(pi.hProcess, INFINITE);
  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);

  return 0;
}

int run(            const char *program)                                           { return run_args_stdinout(program, NULL, NULL, NULL); }
int run_args(       const char *program, const char *args)                         { return run_args_stdinout(program, args, NULL, NULL); }
int run_stdin(      const char *program,                   void *sin)              { return run_args_stdinout(program, NULL, sin,  NULL); }
int run_args_stdin( const char *program, const char *args, void *sin)              { return run_args_stdinout(program, args, sin,  NULL); }
int run_stdout(     const char *program,                              void **sout) { return run_args_stdinout(program, NULL, NULL, sout); }
int run_args_stdout(const char *program, const char *args,            void **sout) { return run_args_stdinout(program, args, NULL, sout); }
int run_stdinout(   const char *program,                   void *sin, void **sout) { return run_args_stdinout(program, NULL, sin,  sout); }

