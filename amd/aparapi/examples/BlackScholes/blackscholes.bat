java ^
 -Djava.library.path=%LIBAPARAPI% ^
 -classpath %LIBAPARAPI%/aparapi.jar;../../AparapiUtil/AparapiUtil.jar;BlackScholes.jar ^
 com.amd.aparapi.sample.BlackScholes.BlackScholes %*

