@echo off
set AVR=C:\Program Files\Arduino\hardware\tools\avr
"%AVR%\bin\avr-gcc" -g -Wall -O2 -mmcu=atmega328p -c -o q7c.o q7c.c -DF_CPU=16000000 -std=c99 --save-temps
"%AVR%\bin\avr-gcc" -g -Wall -O3 -mmcu=atmega328p -c -o q7.o q7.s --save-temps
"%AVR%\bin\avr-gcc" -g -Wall -O3 -mmcu=atmega328p -o q7.elf q7.o q7c.o --save-temps -Wl,-Map,q7.map -Wl,--cref
rem "%AVR%\bin\avr-as" -mmcu=atmega328p -o q7.o q7.s
rem "%AVR%\bin\avr-ld" -m avr5 -Tdata 0x800100 -o q7.elf "%AVR%\avr\lib\avr5\crtm328p.o" "-L%AVR%\lib\gcc\avr\4.3.2\avr5" "-L%AVR%\avr\lib\avr5" "-L%AVR%\lib\gcc\avr\4.3.2" "-L%AVR%\lib\gcc" "-L%AVR%\avr\lib" -Map q7.map q7.o q7c.o -lgcc -lc -lgcc
"%AVR%\bin\avr-objdump" -h -S q7.elf > q7.lst
"%AVR%\bin\avr-objcopy" -j .text -j .data -O ihex q7.elf q7.hex
"%AVR%\bin\avr-nm" -n q7.elf >q7.nm

