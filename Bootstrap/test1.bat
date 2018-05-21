@echo off

if not exist 1.com call build1.bat

echo Testing step 1

rem Hex translation table for 1.com
rem 0123456789ABCDEF
rem 0123456789jklmno

1 25 > 1t.com
1 20 >> 1t.com
1 20 >> 1t.com
1 25 >> 1t.com
1 40 >> 1t.com
1 40 >> 1t.com
1 35 >> 1t.com
1 6k >> 1t.com
1 41 >> 1t.com
1 35 >> 1t.com
1 20 >> 1t.com
1 40 >> 1t.com
1 50 >> 1t.com
1 5k >> 1t.com
1 35 >> 1t.com
1 4k >> 1t.com
1 4o >> 1t.com
1 2n >> 1t.com
1 28 >> 1t.com
1 27 >> 1t.com
1 35 >> 1t.com
1 6k >> 1t.com
1 4o >> 1t.com
1 50 >> 1t.com
1 5k >> 1t.com
1 2m >> 1t.com
1 7n >> 1t.com
1 55 >> 1t.com
1 50 >> 1t.com
1 5o >> 1t.com
1 2n >> 1t.com
1 29 >> 1t.com
1 3o >> 1t.com
1 43 >> 1t.com
1 43 >> 1t.com
1 43 >> 1t.com
1 43 >> 1t.com
1 2m >> 1t.com
1 4l >> 1t.com
1 58 >> 1t.com
1 50 >> 1t.com
1 2n >> 1t.com
1 28 >> 1t.com
1 27 >> 1t.com
1 2m >> 1t.com
1 27 >> 1t.com
1 52 >> 1t.com
1 50 >> 1t.com
1 5k >> 1t.com
1 58 >> 1t.com
1 2n >> 1t.com
1 28 >> 1t.com
1 27 >> 1t.com
1 2m >> 1t.com
1 31 >> 1t.com
1 23 >> 1t.com
1 2m >> 1t.com
1 6l >> 1t.com
1 30 >> 1t.com
1 50 >> 1t.com
1 59 >> 1t.com
1 50 >> 1t.com
1 5j >> 1t.com
1 2m >> 1t.com
1 2o >> 1t.com
1 40 >> 1t.com
1 2m >> 1t.com
1 33 >> 1t.com
1 41 >> 1t.com
1 35 >> 1t.com
1 20 >> 1t.com
1 7n >> 1t.com
1 50 >> 1t.com
1 5k >> 1t.com
1 2n >> 1t.com
1 20 >> 1t.com
1 27 >> 1t.com
1 43 >> 1t.com
1 2n >> 1t.com
1 32 >> 1t.com
1 37 >> 1t.com
1 52 >> 1t.com
1 58 >> 1t.com
1 25 >> 1t.com
1 20 >> 1t.com
1 2o >> 1t.com
1 25 >> 1t.com
1 40 >> 1t.com
1 4o >> 1t.com
1 50 >> 1t.com
1 5j >> 1t.com
1 4k >> 1t.com
1 2n >> 1t.com
1 30 >> 1t.com
1 37 >> 1t.com
1 25 >> 1t.com
1 20 >> 1t.com
1 20 >> 1t.com
1 35 >> 1t.com
1 23 >> 1t.com
1 30 >> 1t.com
1 35 >> 1t.com
1 21 >> 1t.com
1 30 >> 1t.com
1 2n >> 1t.com
1 32 >> 1t.com
1 27 >> 1t.com
1 73 >> 1t.com
1 70 >> 1t.com
1 50 >> 1t.com
1 5j >> 1t.com
1 20 >> 1t.com
1 21 >> 1t.com
1 25 >> 1t.com
1 20 >> 1t.com
1 20 >> 1t.com
1 25 >> 1t.com
1 40 >> 1t.com
1 40 >> 1t.com
1 35 >> 1t.com
1 20 >> 1t.com
1 4l >> 1t.com
1 20 >> 1t.com
1 21 >> 1t.com

rem echo adds these to the end

1 20 >> 1t.com
1 0m >> 1t.com
1 0j >> 1t.com

fc /b 1t.com 1.com >nul
if errorlevel 1 goto :fail
del 1t.com

echo Step 1 completed successfully.
goto :end
:fail
echo Step 1 failed!
:end
