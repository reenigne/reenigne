@echo off

if not exist 2.com call build2.bat

echo Testing step 2

2 0E       > 2t.com
2 1F       >> 2t.com
2 8A0E8000 >> 2t.com
2 BE8100   >> 2t.com
2 32ED     >> 2t.com
2 32D2     >> 2t.com
2 AC       >> 2t.com
2 FEC9     >> 2t.com
2 7C39     >> 2t.com
2 3C30     >> 2t.com
2 7CF7     >> 2t.com
2 3C39     >> 2t.com
2 7E14     >> 2t.com
2 3C41     >> 2t.com
2 7CEF     >> 2t.com
2 3C46     >> 2t.com
2 7E0A     >> 2t.com
2 3C61     >> 2t.com
2 7CE7     >> 2t.com
2 3C66     >> 2t.com
2 7FE3     >> 2t.com
2 2C20     >> 2t.com
2 2C07     >> 2t.com
2 2C30     >> 2t.com
2 D0E2     >> 2t.com
2 D0E2     >> 2t.com
2 D0E2     >> 2t.com
2 D0E2     >> 2t.com
2 08C2     >> 2t.com
2 80F501   >> 2t.com
2 75CE     >> 2t.com
2 B402     >> 2t.com
2 51       >> 2t.com
2 52       >> 2t.com
2 56       >> 2t.com
2 CD21     >> 2t.com
2 5E       >> 2t.com
2 5A       >> 2t.com
2 59       >> 2t.com
2 EBC2     >> 2t.com
2 B44C     >> 2t.com
2 CD21     >> 2t.com

fc /b 2t.com 2.com >nul
if errorlevel 1 goto :fail
del 2t.com

echo Step 2 completed successfully.
goto :end
:fail
echo Step 2 failed!
:end
