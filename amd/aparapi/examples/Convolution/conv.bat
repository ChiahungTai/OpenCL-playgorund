java ^
 -Djava.library.path=%LIBAPARAPI% ^
 -classpath %LIBAPARAPI%/aparapi.jar;../../AparapiUtil/AparapiUtil.jar;Convolution.jar ^
 com.amd.aparapi.sample.Convolution.Convolution %*

