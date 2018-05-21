@echo off

if not exist 5.com call build5.bat

echo Testing step 5

5 5.5 5t.com
fc /b 5t.com 5.com >nul
if errorlevel 1 goto :fail
del 5t.com

echo Step 5 completed successfully.
goto :end
:fail
echo Step 5 failed!
:end
