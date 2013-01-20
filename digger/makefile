digger.exe: main.obj digger.obj monster.obj bags.obj sound.obj newsnd.obj \
            record.obj drawing.obj scores.obj input.obj sprite.obj dospc.obj \
            cgagrafx.obj vgagrafx.obj alpha.obj title.obj ini.obj \
            response.rsp newsnd2.obj
  tlink @response.rsp

digleved.exe: digleved.obj vgagrafx.obj alpha.obj cgagrafx.obj \
              eddospc.obj digleved.rsp
  tlink @digleved.rsp

.asm.obj:
  a86 +o +c +S $<

.c.obj:
  bcc -2- -v -f- -ms -I\prog\include -Z -O -Oa -Ob -Oc -Oe -Og -Oi -Ol -Om \
      -Op -Ov -c -d -w -RT- -k- $<

main.obj: main.c def.h hardware.h sound.h sprite.h input.h scores.h drawing.h \
          digger.h monster.h bags.h record.h main.h newsnd.h ini.h

digger.obj: digger.c def.h sprite.h input.h hardware.h digger.h drawing.h \
            main.h sound.h monster.h scores.h bags.h

monster.obj: monster.c def.h monster.h main.h sprite.h digger.h drawing.h \
             bags.h sound.h scores.h

bags.obj: bags.c def.h bags.h main.h sprite.h sound.h drawing.h monster.h \
          digger.h scores.h

sound.obj: sound.c def.h sound.h hardware.h scores.h

newsnd.obj: newsnd.c def.h device.h hardware.h digger.h sound.h

newsnd2.obj: newsnd2.c def.h device.h

record.obj: record.c def.h record.h hardware.h

drawing.obj: drawing.c def.h drawing.h main.h hardware.h sprite.h

scores.obj: scores.c def.h scores.h main.h drawing.h hardware.h sound.h \
            sprite.h input.h digger.h

input.obj: input.c def.h main.h sound.h

sprite.obj: sprite.c def.h sprite.h hardware.h

ini.obj: ini.c def.h

dospc.obj: dospc.asm

digleved.obj: digleved.c def.h edasm.h

eddospc.obj: eddospc.asm

title.obj: title.c def.h
  bcc -2- -v -f- -mh -I\prog\include -Z -O -c -w -RT- -k- title.c

alpha.obj: alpha.c def.h
  bcc -2- -v -f- -mh -w-sus -I\prog\include -Z -O -c -w -RT- -k- alpha.c

cgagrafx.obj: cgagrafx.c def.h
  bcc -2- -v -f- -mh -w-sus -I\prog\include -Z -O -c -w -RT- -k- cgagrafx.c

vgagrafx.obj: vgagrafx.c def.h
  bcc -2- -v -f- -mh -w-sus -I\prog\include -Z -O -c -w -RT- -k- vgagrafx.c

alpha.c: vgatext.spr cgatext.spr mkg.exe
  mkg alpha.c

cgagrafx.c: cga.spr mkg.exe
  mkg cgagrafx.c

vgagrafx.c: vga.spr mkg.exe
  mkg vgagrafx.c

mkg.exe: mkg.obj
  tlink /x /c \prog\lib\bc0s.obj mkg.obj, mkg.exe,, \prog\lib\bcs.lib
  del chklist.ms

mkg.obj: mkg.c def.h

clean:
  del *.bak
  del *.obj
  del *.map
  del *.sym
  del *.exe
  del vgagrafx.c
  del cgagrafx.c
  del alpha.c
  del chklist.ms
  del *.tr
  del *.td
