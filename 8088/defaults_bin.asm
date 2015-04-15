org 0
%include "defaults_common.asm"

%define captureScreen  int 0x60
%define startAudio     int 0x61
%define stopAudio      int 0x62
%define outputHex      int 0x63
%define sendFile       int 0x66
%define complete       int 0x67
%define loadData       int 0x68
%define stopScreen     int 0x69
%define resumeScreen   int 0x6a
%define stopKeyboard   int 0x6b
%define resumeKeyboard int 0x6c

%macro outputString 0
  int 0x64
%endmacro

%macro outputCharacter 0
  int 0x65
%endmacro

; Write a ^Z character to tell the "run" program to finish
%macro disconnect 0
  outputCharacter 26
  outputCharacter 26
%endmacro

