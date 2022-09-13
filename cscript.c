#include "cscript.h"

#ifdef _WIN32
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

cscript_state css;

void *arena_malloc(size_t s, cscript_arena *arena)
{
  assert(arena->m && arena->size);
  if(s%4) s = (s/4+1)*4; //always align 4
  if(arena->used+s > arena->size)
  {
    size_t new_s = arena->size*2;
    size_t new_s_mb = new_s/1024/1024;
    fprintf(stderr, "REALLOCING ARENA- RECOMMENDED TO INIT TO AT LEAST %lluMB FOR FUTURE BUILDS\n",new_s_mb);
    void *re = realloc(arena->m, arena->size*2);
    if(!re)
    {
      fprintf(stderr, "FAILED TO REALLOC ARENA- INIT TO AT LEAST %lluMB FOR FUTURE BUILDS\n",new_s_mb);
      exit(1);
    }
    if(re != arena->m)
    {
      fprintf(stderr, "REALLOCED ARENA MOVED; CERTAIN TO RESULT IN OUTDATED POINTERS; INIT TO AT LEAST %lluMB AND RE-BUILD\n",new_s_mb);
      exit(1);
      arena->m = re;
    }
    arena->size = new_s;
  }
  void *r = arena->m+arena->used;
  arena->used += s;
  return r;
}

int cscript_init_size(size_t mb)
{
  css.arena.size = 1024*1024*mb;
  css.arena.used = 0;
  css.arena.m = (char *)malloc(css.arena.size);
  get_cwd(&css.iwd);

  return css.arena.size;
}

int cscript_cleanup()
{
  assert(css.arena.size);
  free(css.arena.m);
  css.arena.m = NULL;
  return 0;
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
  *to = malloc(i+1);
  cs_strcpy(from,*to);
  return i;
}
int cs_getstrcat(const char *a, const char *b, char **concat)
{
  int alen = cs_strlen(a);
  int blen = cs_strlen(b);
  *concat = malloc(alen+blen+1);
  cs_strcpy(a,*concat);
  cs_strcpy(b,*(concat+alen));
  return alen+blen;
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
  *str = arena_malloc(18,&css.arena);
  sprintf_s(*str, 18, "%04d%02d%02d%02d%02d%02d%03d", stLocal.wYear, stLocal.wMonth, stLocal.wDay, stLocal.wHour, stLocal.wMinute, stLocal.wSecond, stLocal.wMilliseconds);
  return 17;
}

//fs
int create_file(const char *file, const char *contents)
{
  FILE *f;
  if(fopen_s(&f,file,"w"))
  {
    fprintf(stderr, "failed to open file for writing %s\n",file);
    exit(1);
  }
  int r = fputs(contents, f);
  if(r == EOF)
  {
    fprintf(stderr, "error writing string to file %s\n%s\n",file,contents);
    exit(1);
  }
  fclose(f);
  return r;
}
int append_file(const char *file, const char *contents)
{
  FILE *f;
  if(fopen_s(&f,file,"a"))
  {
    fprintf(stderr, "failed to open file for appending %s\n",file);
    exit(1);
  }
  int r = fputs(contents, f);
  if(r == EOF)
  {
    fprintf(stderr, "error appending string to file %s\n%s\n",file,contents);
    exit(1);
  }
  fclose(f);
  return r;
}
int move_file(const char *from, const char *to)
{
  if(MoveFile(from,to)) return 1;
  else
  {
    fprintf(stderr, "error moving file %s to %s\n",from,to);
    exit(1);
  }
  return 0;
}
int copy_file(const char *from, const char *to)
{
  if(CopyFile(from,to,FALSE)) return 1;
  else
  {
    fprintf(stderr, "error copying file %s to %s\n",from,to);
    exit(1);
  }
  return 0;
}
int delete_file(const char *file)
{
  if(DeleteFile(file)) return 1;
  else
  {
    fprintf(stderr, "error deleting file %s\n",file);
    exit(1);
  }
  return 0;
}
int create_folder(const char *folder)
{
  if(CreateDirectory(folder,NULL)) return 1;
  else
  {
    switch(GetLastError())
    {
      case ERROR_ALREADY_EXISTS: fprintf(stderr, "error creating folder %s\nfolder already exists\n",folder); break;
      case ERROR_PATH_NOT_FOUND: fprintf(stderr, "error creating folder %s\none or more intermediate directories do not exist\n",folder); break;
    }
    exit(1);
  }
  return 0;
}
int move_folder(const char *from, const char *to)
{
  if(MoveFileEx(from, to, MOVEFILE_WRITE_THROUGH)) return 1;
  else
  {
    fprintf(stderr, "error moving folder %s to %s\n",from,to);
    exit(1);
  }
  return 0;
}
int copy_folder(const char *from, const char *to)
{
  create_folder(to);
  char *frombase; cs_getstrcat(from,"\\",&frombase);
  char *tobase;   cs_getstrcat(to,  "\\",&tobase);

  WIN32_FIND_DATA fdata; 
  HANDLE find = NULL; 
  char *filemask; cs_getstrcat(frombase,"*.*",&filemask);
  if((find = FindFirstFile(filemask, &fdata)) == INVALID_HANDLE_VALUE) 
  { 
    fprintf(stderr,"error copying folder %s to %s\n",from,to); 
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
      cs_getstrcat(frombase,fdata.cFileName,&fromfile);
      cs_getstrcat(tobase,fdata.cFileName,&tofile);

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
  char *folderbase; cs_getstrcat(folder,"\\",&folderbase);

  WIN32_FIND_DATA fdata; 
  HANDLE find = NULL; 
  char *filemask; cs_getstrcat(folderbase,"*.*",&filemask);
  if((find = FindFirstFile(filemask, &fdata)) == INVALID_HANDLE_VALUE) 
  { 
    fprintf(stderr,"error deleting folder %s\n",folder); 
    exit(1);
  } 

  char *file;
  do
  { 
         if(cs_strcmp(fdata.cFileName, "." ) == 0) ;
    else if(cs_strcmp(fdata.cFileName, "..") == 0) ;
    else
    { 
      cs_getstrcat(folderbase,fdata.cFileName,&file);

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
    fprintf(stderr,"error deleting folder %s\n",folder); 
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
    fprintf(stderr, "failed to get current working directory\n");
    exit(1);
  }
  *folder = arena_malloc(needed_cwd_len,&css.arena);
  if(GetCurrentDirectory(needed_cwd_len,*folder) != needed_cwd_len-1)
  {
    fprintf(stderr, "inconsistent working directory length\n");
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
  char *folderbase; cs_getstrcat(folder,"\\",&folderbase);

  WIN32_FIND_DATA fdata; 
  HANDLE find = NULL; 
  char *filemask; cs_getstrcat(folderbase,"*.*",&filemask);
  if((find = FindFirstFile(filemask, &fdata)) == INVALID_HANDLE_VALUE) 
  { 
    fprintf(stderr,"error getting files in folder %s\n",folder); 
    exit(1);
  } 

  size_t max = 32;
  *files = arena_malloc(sizeof(char *)*max, &css.arena);
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
          char **newfiles = arena_malloc(sizeof(char *)*max, &css.arena);
          for(int i = 0; i < n; i++)
            newfiles[i] = (*files)[i];
          *files = newfiles;
        }
        cs_getstrcat(folderbase,fdata.cFileName,&((*files)[n]));
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
  char *folderbase; cs_getstrcat(folder,"\\",&folderbase);

  WIN32_FIND_DATA fdata; 
  HANDLE find = NULL; 
  char *filemask; cs_getstrcat(folderbase,"*.*",&filemask);
  if((find = FindFirstFile(filemask, &fdata)) == INVALID_HANDLE_VALUE) 
  { 
    fprintf(stderr,"error getting folders in folder %s\n",folder); 
    exit(1);
  } 

  size_t max = 32;
  *folders = arena_malloc(sizeof(char *)*max, &css.arena);
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
          char **newfolders = arena_malloc(sizeof(char *)*max, &css.arena);
          for(int i = 0; i < n; i++)
            newfolders[i] = (*folders)[i];
          *folders = newfolders;
        }
        cs_getstrcat(folderbase,fdata.cFileName,&((*folders)[n]));
        n++;
      }
    }
  } 
  while(FindNextFile(find, &fdata));

  FindClose(find);

  return n;
}
long long unsigned int file_last_access(const char *file)
{
  long long unsigned int t;
  HANDLE f;

  f = CreateFile(file, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
  if(f == INVALID_HANDLE_VALUE)
    return 0;

  FILETIME ft;
  if(!GetFileTime(f, NULL, NULL, &ft))
  {
    fprintf(stderr,"failed to get access time for file %s\n",file);
    exit(1);
    return 0;
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

int run_args_stdinout(const char *program, const char *args, void *sin, void **sout)
{
  SECURITY_ATTRIBUTES sa;
  memset(&sa,0,sizeof(sa));
  sa.nLength = sizeof(sa);
  sa.bInheritHandle = TRUE;
  sa.lpSecurityDescriptor = NULL;

  HANDLE rsout;
  HANDLE wsout;
  if(!CreatePipe(&rsout, &wsout, &sa, 0)) return HRESULT_FROM_WIN32(GetLastError());
  if(!SetHandleInformation(rsout, HANDLE_FLAG_INHERIT, 0)) return HRESULT_FROM_WIN32(GetLastError());

  HANDLE rsin;
  HANDLE wsin;
  if(!CreatePipe(&rsin, &wsin, &sa, 0)) return HRESULT_FROM_WIN32(GetLastError());
  if(!SetHandleInformation(wsin, HANDLE_FLAG_INHERIT, 0)) return HRESULT_FROM_WIN32(GetLastError());

  PROCESS_INFORMATION pi;
  memset(&pi,0,sizeof(pi));
 
  STARTUPINFO si;     
  memset(&si,0,sizeof(si));
  si.cb = sizeof(si);
  si.hStdError = wsout;
  si.hStdOutput = wsout;
  si.hStdInput = rsin;
  si.dwFlags |= STARTF_USESTDHANDLES;

  char *cargs;
  if(args)
  {
    cs_getstrcat(program," ",&cargs);
    cs_getstrcat(cargs,args,&cargs);
  }
  else
    cs_getstrcpy(program,&cargs);

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
    //error
  }
   
  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);
  
  CloseHandle(wsout);
  CloseHandle(rsin);

  return 0;
}

int run(            const char *program)                                           { return run_args_stdinout(program, NULL, NULL, NULL); }
int run_args(       const char *program, const char *args)                         { return run_args_stdinout(program, args, NULL, NULL); }
int run_stdin(      const char *program,                   void *sin)              { return run_args_stdinout(program, NULL, sin,  NULL); }
int run_args_stdin( const char *program, const char *args, void *sin)              { return run_args_stdinout(program, args, sin,  NULL); }
int run_stdout(     const char *program,                              void **sout) { return run_args_stdinout(program, NULL, NULL, sout); }
int run_args_stdout(const char *program, const char *args,            void **sout) { return run_args_stdinout(program, args, NULL, sout); }
int run_stdinout(   const char *program,                   void *sin, void **sout) { return run_args_stdinout(program, NULL, sin,  sout); }

#endif //_WIN32

#ifdef __unix__
#endif //__unix__
#ifdef __APPLE__
#endif //__APPLE__

/*
int main(int argc, const char **argv)
{
}
*/
