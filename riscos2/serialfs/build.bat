@echo off
set ARM_TOOLS=C:\Program Files (x86)\GNU Tools ARM Embedded\4.9 2015q2
"%ARM_TOOLS%\bin\arm-none-eabi-gcc" -mcpu=arm2 -Wall -O3 -c -o serialfs.o serialfs.c -std=c99 --save-temps -mabi=apcs-gnu -mno-thumb-interwork -fpic
"%ARM_TOOLS%\bin\arm-none-eabi-gcc" -mcpu=arm2 -Wall -O3 -c -o _start.o _start.s -std=c99 --save-temps -mabi=apcs-gnu -mno-thumb-interwork -fpic
"%ARM_TOOLS%\bin\arm-none-eabi-gcc" -mcpu=arm2 -Wall -O3 -o serialfs.elf _start.o serialfs.o --save-temps -Wl,-Map,serialfs.map -nostdlib
"%ARM_TOOLS%\bin\arm-none-eabi-objdump" -h -S serialfs.elf > serialfs.lst
"%ARM_TOOLS%\bin\arm-none-eabi-objcopy" -O binary serialfs.elf serialfs.bin
"%ARM_TOOLS%\bin\arm-none-eabi-nm" -n serialfs.elf >serialfs.nm

