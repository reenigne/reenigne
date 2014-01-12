org 0
%include "defaults_common.asm"

%define captureScreen  int 0x60
%define startAudio     int 0x61
%define stopAudio      int 0x62
%define printHex       int 0x63
%define printString    int 0x64
%define sendFile       int 0x66
%define complete       int 0x67
%define loadSerialData int 0x68
%define stopScreen     int 0x69
%define resumeScreen   int 0x6a

%macro printCharacter 0
  int 0x65
%endmacro

; Write a ^Z character to tell the "run" program to finish
%macro disconnect 0
  printCharacter 26
%endmacro

