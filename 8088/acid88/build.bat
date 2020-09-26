yasm acid88.asm -o a.com -l acid88.lst
copy /b /y a.com+..\xtce\gentests\tests.bin acid88.com
del a.com
