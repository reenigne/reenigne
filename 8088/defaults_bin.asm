org 0
%include "defaults_common.asm"


%define writeHex       int 0x60
%define writeString    int 0x61
%define writeCharacter int 0x62
%define printHex       int 0x63
%define printString    int 0x64
%define printCharacter int 0x65
%define complete       int 0x67
%define loadSerialData int 0x68


%macro writeNewLine 0
  mov al,10
  writeCharacter
%endmacro


%macro printNewLine 0
  mov al,10
  printCharacter
%endmacro


%macro disconnect 0
  mov al,26
  writeCharacter  ; Write a ^Z character to tell the "run" program to finish
%endmacro


%macro print 1+
    jmp %%overMessage
  %%message:
    db %1
  %%overMessage:
    mov si,%%message
    mov cx,%%overMessage - %%message
    printString
%endmacro

