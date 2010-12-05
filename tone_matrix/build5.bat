@echo off
set AVR=C:\Program Files\Arduino\hardware\tools\avr
"%AVR%\bin\avr-gcc" -g -Wall -O2 -mmcu=atmega328p -c -o v2c.o v2c.c -DF_CPU=16000000 -std=c99 --save-temps
"%AVR%\bin\avr-gcc" -g -Wall -O3 -mmcu=atmega328p -c -o v2.o v2.s --save-temps
"%AVR%\bin\avr-gcc" -g -Wall -O3 -mmcu=atmega328p -o v2.elf v2.o v2c.o --save-temps -Wl,-Map,v2.map -Wl,--cref
rem "%AVR%\bin\avr-as" -mmcu=atmega328p -o v2.o v2.s
rem "%AVR%\bin\avr-ld" -m avr5 -Tdata 0x800100 -o v2.elf "%AVR%\avr\lib\avr5\crtm328p.o" "-L%AVR%\lib\gcc\avr\4.3.2\avr5" "-L%AVR%\avr\lib\avr5" "-L%AVR%\lib\gcc\avr\4.3.2" "-L%AVR%\lib\gcc" "-L%AVR%\avr\lib" -Map v2.map v2.o v2c.o -lgcc -lc -lgcc
"%AVR%\bin\avr-objdump" -h -S v2.elf > v2.lst
"%AVR%\bin\avr-objcopy" -j .text -j .data -O ihex v2.elf v2.hex
"%AVR%\bin\avr-nm" -n v2.elf >v2.nm

