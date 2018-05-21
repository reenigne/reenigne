@echo off

rem The test for step 15 is a little more complicated than the tests for previous steps.
rem The reason for this is that 15.com cannot compile itself because the function argument
rem order in the 15 ABI is the opposite from that in the 14 ABI.
rem So we use 15.com to compile an intermediate compiler (15t.com) which is very similar to
rem 15.com but which uses the opposite argument order. This is then tested to make sure it
rem can compile itself.

if not exist 15.com call build15.bat

echo Testing step 15

15 15util.15 15lex.15 15parse.15 15symbol.15 15expr.15 15string.15 15malloc.15 15t.com
15t 15util.15 15lex.15 15parse.15 15symbol.15 15expr.15 15string.15 15malloc.15 15s.com
fc /b 15t.com 15s.com >nul
if errorlevel 1 goto :fail
del 15t.com
del 15s.com

echo Step 15 completed successfully.
goto :end
:fail
echo Step 15 failed!
:end
