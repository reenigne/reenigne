@echo off

if not exist 12.com call build12.bat

echo Testing step 12

12 12.12 12t.com
fc /b 12t.com 12.com >nul
if errorlevel 1 goto :fail
del 12t.com

echo Step 12 completed successfully.
goto :end
:fail
echo Step 12 failed!
:end
