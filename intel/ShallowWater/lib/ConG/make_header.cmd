@echo off

SET infile=%1
SET outfile=%2

if "%STDOUT_REDIRECTED%" == "" (
    	@echo on
 @rem   fxc.exe /Fo "%outfile%.o" /T fx_4_0 /Ges "%infile%"
 @rem   fxc.exe /Cc /T fx_4_0 /Ges "%infile%"
     @echo off

    IF EXIST %outfile% DEL %outfile%
    set STDOUT_REDIRECTED=yes
    cmd.exe /c %0 %* > %outfile%
    exit /b %ERRORLEVEL%
)

 
FOR /F "delims=" %%G IN ('more %infile%') DO (
  SET line=%%G

  SETLOCAL enabledelayedexpansion

  SET line=!line:\=\\!
  SET line=!line:"=\"!
  ECHO "!line!\n"

  ENDLOCAL
)
