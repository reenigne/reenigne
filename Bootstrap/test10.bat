@echo off

if not exist 10.com call build10.bat

echo Testing step 10

10 10.10 10t.com
fc /b 10t.com 10.com >nul
if errorlevel 1 goto :fail
del 10t.com

10 test.10 test10.dat >test10.txt
fc /b test10.txt expected10.txt >nul
if errorlevel 1 goto :fail
del test10.dat
del test10.txt

echo Step 10 completed successfully.
goto :end
:fail
echo Step 10 failed!
:end
