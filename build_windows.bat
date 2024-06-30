@ECHO OFF

SET cflags=/nologo /MT /Gm- /Oi /GR- /EHa- /WX /W4 /wd4211 /wd4100 /wd4189 /wd4201 /wd4505
SET lflags=/opt:ref user32.lib gdi32.lib opengl32.lib winmm.lib

where /q cl
IF %ErrorLevel% NEQ 0 (
    CALL "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
)

cl %cflags% /Fewindows /Fmwindows.map /FC /Z7 .\windows.c /link %lflags%
