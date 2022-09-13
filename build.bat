@set CC="C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\Llvm\x64\bin\clang.exe"
@set CCARGS=-Wall
%CC% %CCARGS% -c cscript.c cscript_test.c
%CC% %CCARGS% -o test.exe cscript.o cscript_test.o
del cscript.o
del cscript_test.o
test.exe
del test.exe