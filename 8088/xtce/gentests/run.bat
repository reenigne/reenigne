@echo off
set HOME=C:\Users\Andrew
set DOIT_HOST=prospero
set MSBuildLoadMicrosoftTargetsReadOnly=
set PkgDefApplicationConfigFile=
set VisualStudioDir=
set VisualStudioEdition=
set VisualStudioVersion=
set VSAPPIDDIR=
set VSAPPIDNAME=
set VSLANG=
set VSSKUEDITION=
set
doitclient wcmd xtrun q:\reenigne\8088\xtce\gentests\runtest.bin
rem >runtests.output
