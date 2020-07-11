set PACKAGE=cgaart
set ZIPDIR=%TMPDIR%\%PACKAGE%
mkdir %ZIPDIR%
copy cgaart.txt %ZIPDIR%
copy Release\cgaart.exe %ZIPDIR%
copy default.config %ZIPDIR%
copy ..\..\external\fftw-3.3.4\libfftw3f-3.dll %ZIPDIR%
copy ..\..\external\8088\roms\5788005.u33 %ZIPDIR%
copy ..\..\external\libpng\projects\vstudio\libpng\Release\libpng15.dll %ZIPDIR%
copy ..\..\external\zlib\contrib\vstudio\vc10\Release\zlibvc.dll %ZIPDIR%
copy ..\..\external\cgaart_rose\rose.png %ZIPDIR%

