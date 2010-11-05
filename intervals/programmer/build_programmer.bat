@echo off
set AVR=C:\Program Files\Arduino\hardware\tools\avr
"%AVR%\bin\avr-gcc" -g -Wall -O3 -mmcu=atmega328p -c -o programmer_c.o programmer_c.c -DF_CPU=16000000 -std=c99 --save-temps
"%AVR%\bin\avr-gcc" -g -Wall -O3 -mmcu=atmega328p -c -o programmer.o programmer.s --save-temps
"%AVR%\bin\avr-gcc" -g -Wall -O3 -mmcu=atmega328p -o programmer.elf programmer.o programmer_c.o --save-temps -Wl,-Map,programmer.map -Wl,--cref
"%AVR%\bin\avr-objdump" -h -S programmer.elf > programmer.lst
"%AVR%\bin\avr-objcopy" -j .text -j .data -O ihex programmer.elf programmer.hex
"%AVR%\bin\avr-nm" -n programmer.elf >programmer.nm

