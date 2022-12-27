set PACKAGE=xtce_trace
set ZIPDIR=%TMPDIR%\%PACKAGE%
mkdir %ZIPDIR%
copy release\trace.exe %ZIPDIR%
copy trace.txt %ZIPDIR%
copy test.asm %ZIPDIR%
copy test.com %ZIPDIR%
copy build.bat %ZIPDIR%
xcopy /y /i roms %ZIPDIR%\roms

