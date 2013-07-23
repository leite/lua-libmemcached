#
LIBNAME =lua-libmemcached
LIBSUFFIX =.so

# common prefix 
INCS =-I/usr/local/include -I/usr/include -I/usr/local/include/luvit/luajit -I.
LIBSDIR =-L/usr/local/lib -L/usr/lib

# basic setup
CC =gcc
LIBS =$(LIBSDIR) -lm -lmemcached
OPTS =-O3
LIBOPT =-c
SHARE =-shared
CFLAGS =$(LIBOPT) $(OPTS) $(INCS) -fPIC
CFLAGS_LIB =$(SHARE) $(OPTS) $(LIBS) $(INCS)

# Strip the binary for our prod build
STRIP =strip

# UPX pack to minimise size for our prod build
UPX =upx
UPXFLAG =-9

.PHONY: clean release all

all: $(LIBNAME)$(LIBSUFFIX) $(LIBNAME).o
	[ -f $(LIBNAME).o ] && \rm $(LIBNAME).o

$(LIBNAME)$(LIBSUFFIX): $(LIBNAME).o
	$(CC) -o $@ $^ $(CFLAGS_LIB)

$(LIBNAME).o: $(LIBNAME).c
	$(CC) -o $@ $^ $(CFLAGS)

release: $(LIBNAME)$(LIBSUFFIX)
	$(STRIP) $<
	$(UPX) $(UPXFLAG) $<

clean:
	\rm -f *.o *.dll *.so