@echo off

if not exist 13.com call build13.bat

echo Testing step 13

13 13util.13 13lex.13 13parse.13 13t.com
fc /b 13t.com 13.com >nul
if errorlevel 1 goto :fail
del 13t.com

echo Step 13 completed successfully.
goto :end
:fail
echo Step 13 failed!
:end
