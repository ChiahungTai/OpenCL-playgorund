@echo off
::Script to modify the aparapi.jar path in .classpath file(aparapi/examples) with LIBAPARAPI environment variable.

SETLOCAL ENABLEEXTENSIONS
SETLOCAL DISABLEDELAYEDEXPANSION


set invar=C:/dev/aparapi/aparapi-2012-05-06/aparapi.jar
set LIB_APARAPI_PATH=%LIBAPARAPI%/aparapi.jar

		

FOR /R "." %%G IN (.classpath) DO (

IF EXIST  %%G (

IF EXIST  "%%G.bak" (
copy "%%G.bak" "%%G"
)
copy "%%G" "%%G.bak"


for /f "tokens=1,* delims=]" %%A in ('"type "%%G" |find /n /v """') do (
    set "line=%%B"	
    if defined line (			
			call set "line=echo.%%line:%invar%=%LIB_APARAPI_PATH%%%"
        for /f "delims=" %%X in ('"echo."%%line%%""')  do %%~X >> %%G_new
    ) ELSE echo.
)

IF EXIST %%G_new (
del %%G
move /Y "%%G_new" "%%G" >nul
echo Modified .classpath file with aparapi.jar path" %%G (Backup file created as .classpath.bak)"
)
)
)
echo "LIBAPARAPI PATH SET TO "%LIB_APARAPI_PATH%
