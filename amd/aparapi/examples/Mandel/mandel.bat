java ^
 -Djava.library.path=%LIBAPARAPI% ^
  -classpath %LIBAPARAPI%/aparapi.jar;../../AparapiUtil/AparapiUtil.jar;Mandel.jar ^
 com.amd.aparapi.sample.Mandel.Mandel %*

