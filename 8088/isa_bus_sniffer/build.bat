@echo off
set AVR=C:\Program Files (x86)\Arduino\hardware\tools\avr
if not exist "%AVR%" set AVR=C:\Program Files\Arduino\hardware\tools\avr
set OLDPATH=%PATH%
set PATH=%AVR%\..\..\..;%PATH%

"%AVR%\bin\avr-gcc" -g -Wall -O3 -mmcu=atmega328p -c -o sniffer.o sniffer.s --save-temps
"%AVR%\bin\avr-gcc" -g -Wall -O3 -mmcu=atmega328p -o sniffer.elf sniffer.o --save-temps -Wl,-Map,sniffer.map -Wl,--cref
"%AVR%\bin\avr-objdump" -h -S sniffer.elf > sniffer.lst
"%AVR%\bin\avr-objcopy" -j .text -j .data -O ihex sniffer.elf sniffer.hex
"%AVR%\bin\avr-nm" -n sniffer.elf >sniffer.nm

set PATH=%OLDPATH%

