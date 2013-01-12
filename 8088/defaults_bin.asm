org 0
%include "defaults_common.asm"


%define printHex       int 0x63
%define printString    int 0x64
%define complete       int 0x67
%define loadSerialData int 0x68

%macro printCharacter 0
  int 0x65
%endmacro

; Write a ^Z character to tell the "run" program to finish
%define disconnect   printCharacter 26


