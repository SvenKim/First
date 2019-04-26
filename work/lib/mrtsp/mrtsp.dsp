# Microsoft Developer Studio Project File - Name="mrtsp" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102
# TARGTYPE "Win32 (x86) Console Application" 0x0103
# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=mrtsp - Win32 Debug Dynamic Library
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "mrtsp.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "mrtsp.mak" CFG="mrtsp - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "mrtsp - Win32 Debug Dynamic Library" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "mrtsp - Win32 Release Dynamic Library" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "mrtsp - Win32 Debug Static Library" (based on "Win32 (x86) Static Library")
!MESSAGE "mrtsp - Win32 Release Static Library" (based on "Win32 (x86) Static Library")
!MESSAGE "mrtsp - Win32 Debug Component" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "mrtsp - Win32 Release Component" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "mrtsp - Win32 Debug Media Channels" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "mrtsp - Win32 Release Media Channels" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "mrtsp - Win32 Debug Console Application" (based on "Win32 (x86) Console Application")
!MESSAGE "mrtsp - Win32 Release Console Application" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "mrtsp - Win32 Debug Dynamic Library"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\..\..\..\platforms\win32-x86\bin\"
# PROP Intermediate_Dir "..\..\..\..\platforms\win32-x86\obj\Debug Dynamic Libaray\mrtsp\"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
LIB32=link.exe -lib
CPP=cl.exe
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "mrtsp_EXPORTS" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /Zp1 /MTd /W3 /Gm /GX /ZI /Od /I ".\\" /I "..\\" /I "..\..\..\..\platforms\win32-x86\include" /I "..\..\..\..\platforms\universal\include" /D "_WINDOWS" /D "_USRDLL" /D "mrtsp_EXPORTS" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "PRINTF_EX_ENABLE" /FR /FD /GZ /c
# SUBTRACT CPP /YX /Yc /Yu
MTL=midl.exe
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
RSC=rc.exe
# ADD BASE RSC /l 0x804 /d "_DEBUG"
# ADD RSC /l 0x804 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo /o"..\..\..\..\platforms\win32-x86\obj\Debug Dynamic Libaray\mrtsp\mrtsp.debug.dll.bsc"
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /map:"..\..\..\..\platforms\win32-x86\bin\mrtsp.debug.dll.map" /debug /machine:I386 /def:".\mrtsp.def" /out:"..\..\..\..\platforms\win32-x86\bin\mrtsp.debug.dll" /implib:"..\..\..\..\platforms\win32-x86\lib\mrtsp.debug.dll.lib" /libpath:"..\..\..\..\platforms\win32-x86\lib"
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "mrtsp - Win32 Release Dynamic Library"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\..\..\..\platforms\win32-x86\bin\"
# PROP Intermediate_Dir "..\..\..\..\platforms\win32-x86\obj\Release Dynamic Library\mrtsp\"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
LIB32=link.exe -lib
CPP=cl.exe
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "mrtsp_EXPORTS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /Zp1 /MT /W3 /GX /O2 /I ".\\" /I "..\\" /I "..\..\..\..\platforms\win32-x86\include" /I "..\..\..\..\platforms\universal\include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "mrtsp_EXPORTS" /Fr /FD /c
# SUBTRACT CPP /YX /Yc /Yu
MTL=midl.exe
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
RSC=rc.exe
# ADD BASE RSC /l 0x804 /d "NDEBUG"
# ADD RSC /l 0x804 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo /o"..\..\..\..\platforms\win32-x86\obj\Release Dynamic Library\mrtsp\mrtsp.dll.bsc"
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /map:"..\..\..\..\platforms\win32-x86\bin\mrtsp.dll.map" /machine:I386 /def:".\mrtsp.def" /implib:"..\..\..\..\platforms\win32-x86\lib\mrtsp.dll.lib" /libpath:"..\..\..\..\platforms\win32-x86\lib"
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "mrtsp - Win32 Debug Static Library"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\..\..\..\platforms\win32-x86\lib\"
# PROP Intermediate_Dir "..\..\..\..\platforms\win32-x86\obj\Debug Static Library\mrtsp\"
# PROP Target_Dir ""
LINK32=link.exe
MTL=midl.exe
CPP=cl.exe
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /Zp1 /MTd /W3 /Gm /GX /ZI /Od /I ".\\" /I "..\\" /I "..\..\..\..\platforms\win32-x86\include" /I "..\..\..\..\platforms\universal\include" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /FD /GZ /c
# SUBTRACT CPP /Fr /YX
RSC=rc.exe
# ADD BASE RSC /l 0x804 /d "_DEBUG"
# ADD RSC /l 0x804 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo /o"..\..\..\..\platforms\win32-x86\obj\Debug Static Library\mrtsp\mrtsp.debug.a.bsc"
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"..\..\..\..\platforms\win32-x86\lib\mrtsp.debug.a"

!ELSEIF  "$(CFG)" == "mrtsp - Win32 Release Static Library"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\..\..\..\platforms\win32-x86\lib"
# PROP Intermediate_Dir "..\..\..\..\platforms\win32-x86\obj\Debug Static Library\mrtsp\"
# PROP Target_Dir ""
LINK32=link.exe
MTL=midl.exe
CPP=cl.exe
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /Zp1 /MT /W3 /GX /O2 /I ".\\" /I "..\\" /I "..\..\..\..\platforms\win32-x86\include" /I "..\..\..\..\platforms\universal\include" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /FD /c
# SUBTRACT CPP /YX
RSC=rc.exe
# ADD BASE RSC /l 0x804 /d "NDEBUG"
# ADD RSC /l 0x804 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo /o"..\..\..\..\platforms\win32-x86\obj\Debug Static Library\mrtsp\mrtsp.a.bsc"
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"..\..\..\..\platforms\win32-x86\lib\mrtsp.a"

!ELSEIF  "$(CFG)" == "mrtsp - Win32 Debug Component"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\..\..\..\platforms\win32-x86\bin\"
# PROP Intermediate_Dir "..\..\..\..\platforms\win32-x86\obj\Debug Component\mrtsp\"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
LIB32=link.exe -lib
CPP=cl.exe
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "mrtsp_EXPORTS" /D "_COMPONENT" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /Zp1 /MTd /W3 /Gm /GX /ZI /Od /I ".\\" /I "..\\" /I "..\..\..\..\platforms\win32-x86\include" /I "..\..\..\..\platforms\universal\include" /D "_WINDOWS" /D "_USRDLL" /D "mrtsp_EXPORTS" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "PRINTF_EX_ENABLE" /FR /FD /GZ /c
# SUBTRACT CPP /YX /Yc /Yu
MTL=midl.exe
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
RSC=rc.exe
# ADD BASE RSC /l 0x804 /d "_DEBUG"
# ADD RSC /l 0x804 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo /o"..\..\..\..\platforms\win32-x86\obj\Debug Component\mrtsp\mrtsp.debug.comp.bsc"
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /map:"..\..\..\..\platforms\win32-x86\bin\mrtsp.debug.comp.map" /debug /machine:I386 /def:".\mrtsp.def" /out:"..\..\..\..\platforms\win32-x86\bin\mrtsp.debug.comp" /implib:"..\..\..\..\platforms\win32-x86\lib\mrtsp.debug.comp.lib" /libpath:"..\..\..\..\platforms\win32-x86\lib"
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "mrtsp - Win32 Release Component"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\..\..\..\platforms\win32-x86\bin\"
# PROP Intermediate_Dir "..\..\..\..\platforms\win32-x86\obj\Release Component\mrtsp\"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
LIB32=link.exe -lib
CPP=cl.exe
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "mrtsp_EXPORTS" /D "_COMPONENT" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /Zp1 /MT /W3 /GX /O2 /I ".\\" /I "..\\" /I "..\..\..\..\platforms\win32-x86\include" /I "..\..\..\..\platforms\universal\include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "mrtsp_EXPORTS" /Fr /FD /c
# SUBTRACT CPP /YX /Yc /Yu
MTL=midl.exe
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
RSC=rc.exe
# ADD BASE RSC /l 0x804 /d "NDEBUG"
# ADD RSC /l 0x804 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo /o"..\..\..\..\platforms\win32-x86\obj\Release Component\mrtsp\mrtsp.dll.bsc"
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /map:"..\..\..\..\platforms\win32-x86\bin\mrtsp.comp.map" /machine:I386 /def:".\mrtsp.def" /implib:"..\..\..\..\platforms\win32-x86\lib\mrtsp.comp.lib" /libpath:"..\..\..\..\platforms\win32-x86\lib"
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "mrtsp - Win32 Debug Media Channels"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\..\..\..\platforms\win32-x86\bin\"
# PROP Intermediate_Dir "..\..\..\..\platforms\win32-x86\obj\Debug Media Channels\mrtsp\"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
LIB32=link.exe -lib
CPP=cl.exe
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "mrtsp_EXPORTS" /D "_MEDIA_CHANNELS" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /Zp1 /MTd /W3 /Gm /GX /ZI /Od /I ".\\" /I "..\\" /I "..\..\..\..\platforms\win32-x86\include" /I "..\..\..\..\platforms\universal\include" /D "_WINDOWS" /D "_USRDLL" /D "mrtsp_EXPORTS" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "PRINTF_EX_ENABLE" /FR /FD /GZ /c
# SUBTRACT CPP /YX /Yc /Yu
MTL=midl.exe
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
RSC=rc.exe
# ADD BASE RSC /l 0x804 /d "_DEBUG"
# ADD RSC /l 0x804 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo /o"..\..\..\..\platforms\win32-x86\obj\Debug Media Channels\mrtsp\mrtsp.debug.channels.bsc"
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /map:"..\..\..\..\platforms\win32-x86\bin\mrtsp.debug.channels.map" /debug /machine:I386 /def:".\mrtsp.def" /out:"..\..\..\..\platforms\win32-x86\bin\mrtsp.debug.channels" /implib:"..\..\..\..\platforms\win32-x86\lib\mrtsp.debug.channels.lib" /libpath:"..\..\..\..\platforms\win32-x86\lib"
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "mrtsp - Win32 Release Media Channels"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\..\..\..\platforms\win32-x86\bin\"
# PROP Intermediate_Dir "..\..\..\..\platforms\win32-x86\obj\Release Media Channels\mrtsp\"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
LIB32=link.exe -lib
CPP=cl.exe
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "mrtsp_EXPORTS" /D "_MEDIA_CHANNELS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /Zp1 /MT /W3 /GX /O2 /I ".\\" /I "..\\" /I "..\..\..\..\platforms\win32-x86\include" /I "..\..\..\..\platforms\universal\include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "mrtsp_EXPORTS" /Fr /FD /c
# SUBTRACT CPP /YX /Yc /Yu
MTL=midl.exe
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
RSC=rc.exe
# ADD BASE RSC /l 0x804 /d "NDEBUG"
# ADD RSC /l 0x804 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo /o"..\..\..\..\platforms\win32-x86\obj\Release Media Channels\mrtsp\mrtsp.channels.bsc"
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /map:"..\..\..\..\platforms\win32-x86\bin\mrtsp.channels.map" /machine:I386 /def:".\mrtsp.def" /implib:"..\..\..\..\platforms\win32-x86\lib\mrtsp.channels.lib" /libpath:"..\..\..\..\platforms\win32-x86\lib"
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "mrtsp - Win32 Debug Console Application"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\..\..\..\platforms\win32-x86\bin\"
# PROP Intermediate_Dir "..\..\..\..\platforms\win32-x86\obj\Debug Console Application\mrtsp\"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /Zp1 /MTd /W3 /Gm /GX /ZI /Od /I ".\\" /I "..\\" /I "..\..\..\..\platforms\win32-x86\include" /I "..\..\..\..\platforms\universal\include" /D "_CONSOLE" /D "WIN32" /D "_DEBUG" /D "_MBCS" /Fr /FD /GZ /c
# SUBTRACT CPP /YX /Yc /Yu
RSC=rc.exe
# ADD BASE RSC /l 0x804 /d "_DEBUG"
# ADD RSC /l 0x804 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo /o"..\..\..\..\platforms\win32-x86\obj\mrtsp\mrtsp.debug.exe.bsc"
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /out:"..\..\..\..\platforms\win32-x86\bin\mrtsp.debug.exe" /libpath:"..\..\..\..\platforms\win32-x86\lib"

!ELSEIF  "$(CFG)" == "mrtsp - Win32 Release Console Application"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\..\..\..\platforms\win32-x86\bin\"
# PROP Intermediate_Dir "..\..\..\..\platforms\win32-x86\obj\Release Console Application\mrtsp\"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /Zp1 /MT /W3 /GX /O2 /I ".\\" /I "..\\" /I "..\..\..\..\platforms\win32-x86\include" /I "..\..\..\..\platforms\universal\include" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /FD /c
# SUBTRACT CPP /YX /Yc /Yu
RSC=rc.exe
# ADD BASE RSC /l 0x804 /d "NDEBUG"
# ADD RSC /l 0x804 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo /o"..\..\..\..\platforms\win32-x86\obj\Release Console Application\mrtsp\mrtsp.exe.bsc"
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386  /out:"..\..\..\..\platforms\win32-x86\bin\mrtsp.exe" /libpath:"..\..\..\..\platforms\win32-x86\lib"

!ENDIF 

# Begin Target

# Name "mrtsp - Win32 Debug Dynamic Library"
# Name "mrtsp - Win32 Release Dynamic Library"
# Name "mrtsp - Win32 Debug Static Library"
# Name "mrtsp - Win32 Release Static Library"
# Name "mrtsp - Win32 Debug Component"
# Name "mrtsp - Win32 Release Component"
# Name "mrtsp - Win32 Debug Media Channels"
# Name "mrtsp - Win32 Release Media Channels"
# Name "mrtsp - Win32 Debug Console Application"
# Name "mrtsp - Win32 Release Console Application"

# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\create_sdp.c
# End Source File
# Begin Source File

SOURCE=.\rtp.c
# End Source File
# Begin Source File

SOURCE=.\rtp_data.c
# End Source File
# Begin Source File

SOURCE=.\rtp_packet.c
# End Source File
# Begin Source File

SOURCE=.\rtsp_on_msg.c
# End Source File
# Begin Source File

SOURCE=.\rtsp_session.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\create_sdp.h
# End Source File
# Begin Source File

SOURCE=.\mrtsp.h
# End Source File
# Begin Source File

SOURCE=.\rtp.h
# End Source File
# Begin Source File

SOURCE=.\rtp_cfg.h
# End Source File
# Begin Source File

SOURCE=.\rtp_def.h
# End Source File
# Begin Source File

SOURCE=.\rtp_os.h
# End Source File
# Begin Source File

SOURCE=.\rtp_packet.h
# End Source File
# Begin Source File

SOURCE=.\rtsp_mod.h
# End Source File
# Begin Source File

SOURCE=.\rtsp_session.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=".\mrtsp.def"
# End Source File
# End Group
# Begin Source File

SOURCE=.\ReadMe.txt
# End Source File
# End Target
# End Project
