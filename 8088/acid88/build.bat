yasm acid88.asm -o a.com -l acid88.lst
rem copy /b /y a.com+..\xtce\gentests\tests.bin acid88.com
copy /b /y a.com+rearrange\tests_rearranged.bin acid88.com
del a.com
