@echo off
set AVR=C:\Program Files (x86)\Arduino\hardware
if not exist "%AVR%" set AVR=C:\Program Files\Arduino\hardware
"%AVR%\tools\avr\bin\avrdude" -p m328p -P COM1 -c arduino -U flash:w:keyboard.hex -C "%AVR%\arduino\avr\bootloaders\gemma\avrdude.conf" -b 57600
