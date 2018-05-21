@echo off

if not exist 7.com call build7.bat

echo Testing step 7

7 7.7 7t.com
fc /b 7t.com 7.com >nul
if errorlevel 1 goto :fail
del 7t.com

echo Step 7 completed successfully.
goto :end
:fail
echo Step 7 failed!
:end
