yasm c_sa.asm -o c.bin
copy /b c.bin+calibration_5_OLD.bin+slice_newgradients.bin+slice_''new''.bin c_sa.bin
yasm i.asm -o i.bin
