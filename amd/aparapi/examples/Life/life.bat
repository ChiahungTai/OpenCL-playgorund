java ^
 -Djava.library.path=%LIBAPARAPI% ^
 -classpath %LIBAPARAPI%/aparapi.jar;../../AparapiUtil/AparapiUtil.jar;Life.jar ^
 com.amd.aparapi.sample.Life.Life %*