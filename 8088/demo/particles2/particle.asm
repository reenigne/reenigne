; We want to make one pass over the particles
;   Erase from old position
;     Ideally need to find the topmost particle in the same location
;     For now, let's just erase to background
;   Move
;   Draw at new position
;
; Each particle has:
;   Colour
;   Position (fractional)
;   Velocity (fractional)


; si = particle data pointer
doParticle:
  mov ax,[si]  ; x position
  add ax,[si+2]  ; x velocity
  mov cx,[si+4]  ; y position
  add cx,[si+6]  ; y velocity
  mov bl,ch
  mov bh,yTable >> 8
  add bx,bx
  mov di,[bx]

