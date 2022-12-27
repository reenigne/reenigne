set PACKAGE=xtce_trace
set ZIPDIR=%TMPDIR%\%PACKAGE%
mkdir %ZIPDIR%
copy release\trace.exe %ZIPDIR%
copy trace.txt %ZIPDIR%
xcopy /y /i roms %ZIPDIR%\roms

