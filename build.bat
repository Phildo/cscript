@set CC="C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\Llvm\x64\bin\clang.exe"
@set CCARGS=-g -Wall
%CC% %CCARGS% -c cscript_test.c cscript_selfbuild.c
%CC% %CCARGS% -o test.exe      cscript_test.o
%CC% %CCARGS% -o selfbuild.exe cscript_selfbuild.o
::@test.exe
::@echo DOING IT
::@selfbuild.exe
::@echo DID IT

::del cscript.o
::del cscript_test.o
::del cscript_selfbuild.o
::del test.exe
::del selfbuild.exe