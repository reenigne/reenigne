@echo off
"C:\Program Files\WinAVR-20100110\bin\avrdude" -p m328p -P COM9 -c arduino -U flash:w:sniffer.hex -C "C:\Program Files\WinAVR-20100110\bin\avrdude.conf" -b 57600
