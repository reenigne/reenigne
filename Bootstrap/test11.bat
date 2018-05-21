@echo off

if not exist 11.com call build11.bat

echo Testing step 11

11 11.11 11t.com
fc /b 11t.com 11.com >nul
if errorlevel 1 goto :fail
del 11t.com

echo Step 11 completed successfully.
goto :end
:fail
echo Step 11 failed!
:end
