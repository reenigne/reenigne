org 0
%include "defaults_common.asm"

%macro outputString 0
  int 0x64
%endmacro

%macro outputCharacter 0
  int 0x65
%endmacro


