@echo off
yasm hello.asm -o hello.bin
yasm delay.asm -o delay.bin
yasm halt.asm -o halt.bin
yasm flags.asm -o flags.bin
yasm sweep.asm -o sweep.bin
yasm sweep2.asm -o sweep2.bin
yasm sweep2r.asm -o sweep2r.bin
yasm clicks.asm -o clicks.bin
yasm sweep3.asm -o sweep3.bin
yasm image.asm -o image.bin
yasm bios.asm -o bios.bin
yasm retr.asm -o retr.bin
yasm image_time.asm -o image_time.bin
yasm hres_colours.asm -o hres_colours.bin
yasm vsplit.asm -o vsplit.bin
yasm 7mhz.asm -o 7mhz.bin
yasm indirect_call.asm -o indirect_call.bin
