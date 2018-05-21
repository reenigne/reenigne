@echo off

if not exist 3.com call build3.bat

rem --------------------------------------------------------------------------
rem Step 4: Use the program from step 3 to make an improved hex to binary
rem translator. The aim of this one is to add comments so that the unassembled
rem code can be put in line with the assembled code.
rem --------------------------------------------------------------------------

echo Building step 4

3 4.3 4.com
