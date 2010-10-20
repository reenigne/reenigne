@echo off
set TOOLS=C:\Program Files\Microchip\MPASM Suite
call :build waveform
goto :EOF

:build
"%TOOLS%\MPASMWIN.exe" /q /p12F508 %1.asm /l%1.lst /e%1.err
type %1.err
"%TOOLS%\mplink.exe" /p12F508 %1.o /o%1.cof /M%1.map /W /x
goto :EOF

