tasm test.asm
tlink test.obj, test.com
del test.obj
mtd test.com
del test.com
