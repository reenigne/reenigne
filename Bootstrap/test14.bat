@echo off

if not exist 14.com call build14.bat

echo Testing step 14

14 14util.13 14lex.13 14parse.13 14expr.13 14malloc.13 14t.com
fc /b 14t.com 14.com >nul
if errorlevel 1 goto :fail
del 14t.com

echo Step 14 completed successfully.
goto :end
:fail
echo Step 14 failed!
:end
