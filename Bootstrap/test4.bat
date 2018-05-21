@echo off

if not exist 4.com call build4.bat

echo Testing step 4

4 4.4 4t.com
fc /b 4t.com 4.com >nul
if errorlevel 1 goto :fail
del 4t.com

echo Step 4 completed successfully.
goto :end
:fail
echo Step 4 failed!
:end
