DEPTH = ..

include $(DEPTH)/make/openclsdkdefs.mk 

SUBDIRS = opencl 
SUBDIRS += aparapi 
ifneq ($(OS), lnx)
SUBDIRS += C++Amp
ifeq ($(BITS), 64)
SUBDIRS += bolt
endif
endif

include $(DEPTH)/make/openclsdkrules.mk 

