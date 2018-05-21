@echo off

if not exist 9.com call build9.bat

echo Testing step 9

9 9.9 9t.com
fc /b 9t.com 9.com >nul
if errorlevel 1 goto :fail
del 9t.com

9 test_a.9 test9.dat >test9.txt
fc /b test9.txt expected9a.txt >nul
if errorlevel 1 goto :fail
del test9.dat
del test9.txt

9 test_b.9 test9.dat >test9.txt
fc /b test9.txt expected9b.txt >nul
if errorlevel 1 goto :fail
del test9.dat
del test9.txt

echo Step 9 completed successfully.
goto :end
:fail
echo Step 9 failed!
:end
