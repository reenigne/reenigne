@echo off

if not exist 3.com call build3.bat

echo Testing step 3

3 3.3 3t.com
fc /b 3t.com 3.com >nul
if errorlevel 1 goto :fail
del 3t.com

echo Step 3 completed successfully.
goto :end
:fail
echo Step 3 failed!
:end
