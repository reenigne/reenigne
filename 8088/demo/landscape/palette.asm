org 0x100
cpu 8086

  mov ax,1
  int 0x10
  mov dx,0x3d8
  mov al,0x08
  out dx,al

  mov ax,0xb800
  mov es,ax
  xor di,di

  mov ax,0xb0
  mov dx,16
yLoop:
  mov cx,16
xLoop:
  stosw
  stosw
  inc ah
  loop xLoop
  add di,16
  dec dx
  jnz yLoop

  xor ax,ax
  int 0x16
  ret

; Run this in DOSBox and take a screenshot (Ctrl+F5)
; Find screenshot in %LOCALAPPDATA%\DOSBox\Capture and resize with
; magick palette_000.png -colorspace RGB -scale 25% -colorspace sRGB palette1.png
; Crop and resize to 16x16 in PSP, resulting image will be paletted with palette in correct order
; Save palette
; Find area on Google Maps and turn off labels, capture, crop, resize
; Find elevation data on https://earthexplorer.usgs.gov/ Data set Digital Elevation GMTED2010 is 30m (1 arcsecond) resolution, lat/long aligned
