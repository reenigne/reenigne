@echo off

if not exist 8.com call build8.bat

echo Testing step 8

8 8.8 8t.com
fc /b 8t.com 8.com >nul
if errorlevel 1 goto :fail
del 8t.com

echo Step 8 completed successfully.
goto :end
:fail
echo Step 8 failed!
:end
