# Microsoft Developer Studio Project File - Name="WinDig" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=WinDig - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "WINDIG.MAK".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "WINDIG.MAK" CFG="WinDig - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "WinDig - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "WinDig - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE "WinDig - Win32 No DirectX Release" (based on "Win32 (x86) Application")
!MESSAGE "WinDig - Win32 No DirectX Debug" (based on "Win32 (x86) Application")
!MESSAGE "WinDig - Win32 Combined Debug" (based on "Win32 (x86) Application")
!MESSAGE "WinDig - Win32 Combined Release" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "WinDig - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\Release"
# PROP BASE Intermediate_Dir ".\Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".\Release"
# PROP Intermediate_Dir ".\Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /W3 /Gf /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MSVC" /D "LIBC" /D "DIRECTX" /FD /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x1009 /d "NDEBUG"
# ADD RSC /l 0x1009 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 libc.lib ole32.lib oleaut32.lib dsound.lib ddraw.lib dxguid.lib kernel32.lib user32.lib gdi32.lib comdlg32.lib advapi32.lib shell32.lib uuid.lib winmm.lib comctl32.lib /nologo /subsystem:windows /pdb:none /machine:I386 /nodefaultlib /out:"WinDigDX.exe"

!ELSEIF  "$(CFG)" == "WinDig - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\Debug"
# PROP BASE Intermediate_Dir ".\Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\Debug"
# PROP Intermediate_Dir ".\Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MSVC" /D "LIBC" /D "DIRECTX" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x1009 /d "_DEBUG"
# ADD RSC /l 0x1009 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /debug /machine:I386
# ADD LINK32 winspool.lib ole32.lib oleaut32.lib dsound.lib ddraw.lib dxguid.lib kernel32.lib user32.lib gdi32.lib comdlg32.lib advapi32.lib shell32.lib uuid.lib winmm.lib comctl32.lib libc.lib /nologo /subsystem:windows /debug /machine:I386 /nodefaultlib /out:"WinDigDX.exe"

!ELSEIF  "$(CFG)" == "WinDig - Win32 No DirectX Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\WinDig__"
# PROP BASE Intermediate_Dir ".\WinDig__"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".\NoDXRelease"
# PROP Intermediate_Dir ".\NoDXRelease"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gf /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MSVC" /D "LIBC" /c
# SUBTRACT BASE CPP /YX
# ADD CPP /nologo /W3 /Gf /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MSVC" /D "LIBC" /FD /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x1009 /d "NDEBUG"
# ADD RSC /l 0x1009 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 libc.lib ole32.lib oleaut32.lib dsound.lib ddraw.lib dxguid.lib kernel32.lib user32.lib gdi32.lib comdlg32.lib advapi32.lib shell32.lib uuid.lib winmm.lib comctl32.lib /nologo /subsystem:windows /pdb:none /machine:I386 /nodefaultlib
# ADD LINK32 kernel32.lib user32.lib gdi32.lib comdlg32.lib advapi32.lib shell32.lib winmm.lib comctl32.lib libc.lib /nologo /subsystem:windows /pdb:none /machine:I386 /nodefaultlib /out:"WinDig32.exe"

!ELSEIF  "$(CFG)" == "WinDig - Win32 No DirectX Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\WinDig_0"
# PROP BASE Intermediate_Dir ".\WinDig_0"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\NoDXDebug"
# PROP Intermediate_Dir ".\NoDXDebug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MSVC" /D "LIBC" /FR /YX /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MSVC" /D "LIBC" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x1009 /d "_DEBUG"
# ADD RSC /l 0x1009 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 winspool.lib ole32.lib oleaut32.lib dsound.lib ddraw.lib dxguid.lib kernel32.lib user32.lib gdi32.lib comdlg32.lib advapi32.lib shell32.lib uuid.lib winmm.lib comctl32.lib libc.lib /nologo /subsystem:windows /debug /machine:I386 /nodefaultlib
# ADD LINK32 winspool.lib kernel32.lib user32.lib gdi32.lib comdlg32.lib advapi32.lib shell32.lib winmm.lib comctl32.lib libc.lib /nologo /subsystem:windows /debug /machine:I386 /nodefaultlib /out:"WinDig32.exe"

!ELSEIF  "$(CFG)" == "WinDig - Win32 Combined Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\WinDig__"
# PROP BASE Intermediate_Dir ".\WinDig__"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\CombinedDebug"
# PROP Intermediate_Dir ".\CombinedDebug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MSVC" /D "LIBC" /D "DIRECTX" /FR /YX /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MSVC" /D "LIBC" /D "DIRECTX" /D "RUNTIMEDYNAMICLINK" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x1009 /d "_DEBUG"
# ADD RSC /l 0x1009 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 winspool.lib ole32.lib oleaut32.lib dsound.lib ddraw.lib dxguid.lib kernel32.lib user32.lib gdi32.lib comdlg32.lib advapi32.lib shell32.lib uuid.lib winmm.lib comctl32.lib libc.lib /nologo /subsystem:windows /debug /machine:I386 /nodefaultlib /out:"WinDigDX.exe"
# ADD LINK32 winspool.lib ole32.lib oleaut32.lib kernel32.lib user32.lib gdi32.lib comdlg32.lib advapi32.lib shell32.lib uuid.lib winmm.lib comctl32.lib libc.lib /nologo /subsystem:windows /debug /machine:I386 /nodefaultlib /out:"WinDig.exe"

!ELSEIF  "$(CFG)" == "WinDig - Win32 Combined Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\WinDig__"
# PROP BASE Intermediate_Dir ".\WinDig__"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".\CombinedRelease"
# PROP Intermediate_Dir ".\CombinedRelease"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gf /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MSVC" /D "LIBC" /D "DIRECTX" /c
# SUBTRACT BASE CPP /YX
# ADD CPP /nologo /W3 /Gf /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MSVC" /D "LIBC" /D "DIRECTX" /D "RUNTIMEDYNAMICLINK" /FD /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x1009 /d "NDEBUG"
# ADD RSC /l 0x1009 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 libc.lib ole32.lib oleaut32.lib dsound.lib ddraw.lib dxguid.lib kernel32.lib user32.lib gdi32.lib comdlg32.lib advapi32.lib shell32.lib uuid.lib winmm.lib comctl32.lib /nologo /subsystem:windows /pdb:none /machine:I386 /nodefaultlib /out:"WinDigDX.exe"
# ADD LINK32 libc.lib ole32.lib oleaut32.lib kernel32.lib user32.lib gdi32.lib comdlg32.lib advapi32.lib shell32.lib winmm.lib comctl32.lib /nologo /subsystem:windows /pdb:none /machine:I386 /nodefaultlib /out:"WinDig.exe"

!ENDIF 

# Begin Target

# Name "WinDig - Win32 Release"
# Name "WinDig - Win32 Debug"
# Name "WinDig - Win32 No DirectX Release"
# Name "WinDig - Win32 No DirectX Debug"
# Name "WinDig - Win32 Combined Debug"
# Name "WinDig - Win32 Combined Release"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat;for;f90"
# Begin Source File

SOURCE=.\alpha.c
# End Source File
# Begin Source File

SOURCE=.\Bags.c
# End Source File
# Begin Source File

SOURCE=.\Def.h
# End Source File
# Begin Source File

SOURCE=.\Digger.c
# End Source File
# Begin Source File

SOURCE=.\Drawing.c
# End Source File
# Begin Source File

SOURCE=.\Ini.c
# End Source File
# Begin Source File

SOURCE=.\Input.c
# End Source File
# Begin Source File

SOURCE=.\Main.c
# End Source File
# Begin Source File

SOURCE=.\Monster.c
# End Source File
# Begin Source File

SOURCE=.\Newsnd.c
# End Source File
# Begin Source File

SOURCE=.\Record.c
# End Source File
# Begin Source File

SOURCE=.\Scores.c
# End Source File
# Begin Source File

SOURCE=.\Sound.c
# End Source File
# Begin Source File

SOURCE=.\Sprite.c
# End Source File
# Begin Source File

SOURCE=.\vgagrafx.c
# End Source File
# Begin Source File

SOURCE=.\win_snd.c
# End Source File
# Begin Source File

SOURCE=.\win_sys.c
# End Source File
# Begin Source File

SOURCE=.\win_vid.c
# End Source File
# Begin Source File

SOURCE=.\windig.rc
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl;fi;fd"
# Begin Source File

SOURCE=.\bags.h
# End Source File
# Begin Source File

SOURCE=.\Device.h
# End Source File
# Begin Source File

SOURCE=.\digger.h
# End Source File
# Begin Source File

SOURCE=.\drawing.h
# End Source File
# Begin Source File

SOURCE=.\hardware.h
# End Source File
# Begin Source File

SOURCE=.\Ini.h
# End Source File
# Begin Source File

SOURCE=.\input.h
# End Source File
# Begin Source File

SOURCE=.\main.h
# End Source File
# Begin Source File

SOURCE=.\monster.h
# End Source File
# Begin Source File

SOURCE=.\Newsnd.h
# End Source File
# Begin Source File

SOURCE=.\Record.h
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\scores.h
# End Source File
# Begin Source File

SOURCE=.\sound.h
# End Source File
# Begin Source File

SOURCE=.\sprite.h
# End Source File
# Begin Source File

SOURCE=.\win_dig.h
# End Source File
# Begin Source File

SOURCE=.\Win_snd.h
# End Source File
# Begin Source File

SOURCE=.\Win_vid.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\Digger.ico
# End Source File
# Begin Source File

SOURCE=.\Vtitle.bmp
# End Source File
# End Group
# End Target
# End Project
