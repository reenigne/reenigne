yasm calibrate.asm -o c.com
copy /b c.com+calibration_5_OLD.bin+slice_newgradients.bin+slice_''new''.bin+whichCGA_II_192rows.bin calibrat.com
yasm c_sa.asm -o c.com
copy /b c.com+calibration_5_OLD.bin+slice_newgradients.bin+slice_''new''.bin+whichCGA_II_192rows.bin c_sa.bin

