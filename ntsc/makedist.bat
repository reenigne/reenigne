set PACKAGE=ntsc
set ZIPDIR=%TMPDIR%\%PACKAGE%
mkdir %ZIPDIR%\include\alfe
copy ..\include\alfe\*.h %ZIPDIR%\include\alfe
copy ntsc.txt %ZIPDIR%
copy ..\UNLICENSE %ZIPDIR%
copy avi2ntsc\Release\avi2ntsc.exe %ZIPDIR%
copy ntsc2avi\Release\ntsc2avi.exe %ZIPDIR%
copy ntsc2avi\default.config %ZIPDIR%
copy ..\..\external\fftw-3.3.4\libfftw3f-3.dll %ZIPDIR%
mkdir %ZIPDIR%\src\ntsc2avi
mkdir %ZIPDIR%\src\avi2ntsc
copy avi2ntsc\avi2ntsc.cpp %ZIPDIR%\src\avi2ntsc
copy avi2ntsc\avi2ntsc.sln %ZIPDIR%\src\avi2ntsc
copy avi2ntsc\avi2ntsc.vcxproj %ZIPDIR%\src\avi2ntsc
copy avi2ntsc\avi2ntsc.vcxproj.filters %ZIPDIR%\src\avi2ntsc
copy ntsc2avi\ntsc2avi.cpp %ZIPDIR%\src\ntsc2avi
copy ntsc2avi\ntsc2avi.sln %ZIPDIR%\src\ntsc2avi
copy ntsc2avi\ntsc2avi.vcxproj %ZIPDIR%\src\ntsc2avi
copy ntsc2avi\ntsc2avi.vcxproj.filters %ZIPDIR%\src\ntsc2avi

