@echo off

if not exist 6.com call build6.bat

echo Testing step 6

6 6.6 6t.com
fc /b 6t.com 6.com >nul
if errorlevel 1 goto :fail
del 6t.com

echo Step 6 completed successfully.
goto :end
:fail
echo Step 6 failed!
:end
