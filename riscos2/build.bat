@echo off
set ARM_TOOLS=C:\Program Files (x86)\GNU Tools ARM Embedded\4.9 2015q2
"%ARM_TOOLS%\bin\arm-none-eabi-gcc" -mcpu=arm2 -Wall -O3 -c -o hello.o hello.c -std=c99 --save-temps -mabi=apcs-gnu -mno-thumb-interwork
"%ARM_TOOLS%\bin\arm-none-eabi-gcc" -mcpu=arm2 -Wall -O3 -c -o _start.o _start.s -std=c99 --save-temps -mabi=apcs-gnu -mno-thumb-interwork
"%ARM_TOOLS%\bin\arm-none-eabi-gcc" -mcpu=arm2 -Wall -O3 -o hello.elf _start.o hello.o --save-temps -Wl,-Map,hello.map -nostdlib
"%ARM_TOOLS%\bin\arm-none-eabi-objdump" -h -S hello.elf > hello.lst
"%ARM_TOOLS%\bin\arm-none-eabi-objcopy" -O binary hello.elf hello.bin
"%ARM_TOOLS%\bin\arm-none-eabi-nm" -n hello.elf >hello.nm

