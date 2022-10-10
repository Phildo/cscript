@set CC="C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\Llvm\x64\bin\clang.exe"
@set CCARGS=-g -Wall
%CC% %CCARGS% -c ../platforms/cscript_win.c buildbuild.c
%CC% %CCARGS% -o buildbuild.exe cscript_win.o buildbuild.o 
