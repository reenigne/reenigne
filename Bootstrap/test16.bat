@echo off

rem The test for step 16 is a little more complicated than the tests for previous steps.
rem The reason for this is that 16.com cannot compile itself because it is a 16-bit DOS .com
rem file and it generates 32-bit Windows PE .exe files.
rem So we use 16.com to compile an intermediate compiler (16t.exe) which is very similar to
rem 16.com but which is a 32-bit program.

if not exist 16.com call build16.bat

echo Testing step 16

16 16util.16 16lex.16 16parse.16 16symbol.16 16expr.16 16string.16 16pe.16 16t.exe
16t 16util.16 16lex.16 16parse.16 16symbol.16 16expr.16 16string.16 16pe.16 16s.exe
fc /b 16t.exe 16s.exe >nul
if errorlevel 1 goto :fail
del 16t.exe
del 16s.exe

echo Step 16 completed successfully.
goto :end
:fail
echo Step 16 failed!
:end
