yasm runtests.asm -o runtests.bin
yasm runstub.asm -o runstub.bin -l runstub.lst
yasm functional.asm -o functional.bin -l functional.lst
copy /y functional.bin ..\compare_hardware_lkg
copy /y runstub.bin ..\compare_hardware_lkg
copy /y runtests.bin ..\compare_hardware_lkg
copy /y functional.bin ..\compare_candidate_lkg
copy /y runstub.bin ..\compare_candidate_lkg

