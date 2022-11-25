nasm landscape.asm -o landscape.obj -f obj -l landscape.lst
Q:\Projects\Code\8088mph\tools\msvc\bin\link landscape.obj, landscape.exe, landscape.map,, /NOLOGO /ONERROR:NOEXE /NOI /NOE
Q:\Projects\Code\8088mph\tools\msvc\bin\exehdr /min:0x118c landscape.exe
rem "C:\Program Files (x86)\DOSBox-0.74-3\dosbox-74-3-debug.exe" compress.bat
copy landscape.exe q:\l.exe
