# Some variables needed by the modules
SRCDIR=src

CFLAGS+=-DHAVE_CONFIG_H -Wall -pipe -DHAVE_SVNVERSION_H
#-combine -fwhole-program

ifeq ($(HAVE_SHADERS),1)
	CFLAGS+= -DHAVE_SHADERS
endif

ifeq ($(HAVE_OPENAL),1)
	CFLAGS+= -DHAVE_OPENAL
endif

ifeq ($(HAVE_IPV6),1)
	CFLAGS+= -DHAVE_IPV6
endif

# Common things
_BUILDDIR=$(strip $(BUILDDIR))
_MODULE=$(strip $(MODULE))
_SRCDIR=$(strip $(SRCDIR))

DEP=$(CC) $(CFLAGS) -c $< -MM -MT "$(@:%.d=%.o) $@" -MF $@

LIBTOOL_LD=$(LIBTOOL) --silent --mode=link $(CC) -module -rpath / $(LDFLAGS) $(KIBS)
LIBTOOL_CC=$(LIBTOOL) --silent --mode=compile $(CC) -prefer-pic $(CFLAGS)

# Target options

ifeq ($(BUILD_DEBUG),1)
    BUILDDIR=debug-$(TARGET_OS)-$(TARGET_CPU)
    CFLAGS+=-ggdb -O0 -DDEBUG $(PROFILER_CFLAGS) -fno-inline
else
    BUILDDIR=release-$(TARGET_OS)-$(TARGET_CPU)
    CFLAGS+=-DNDEBUG $(RELEASE_CFLAGS)
endif

ifeq ($(PROFILING),1)
    CFLAGS+=-pg -DPROFILING -fprofile-arcs -ftest-coverage
endif

ifeq ($(PARANOID),1)
    CFLAGS+=-DPARANOID
endif

