TARGET := libsimplevideo.so
OBJS := impl.o
CC := gcc
PLATFORM := $(shell uname -s)


CFLAGS := -std=c99 -fPIC -g
ifeq ($(PLATFORM),Linux)
	# XXX: derive from javac location
	CFLAGS += -I/usr/java/latest/include
	CFLAGS += -I/usr/java/latest/include/linux
endif
ifeq ($(PLATFORM),Darwin)
#	CFLAGS += -arch i386 -arch x86_64
	CFLAGS += -arch x86_64
	CFLAGS += -I/Library/Java/JavaVirtualMachines/jdk1.8.0_51.jdk/Contents/Home/include
	CFLAGS += -I/Library/Java/JavaVirtualMachines/jdk1.8.0_51.jdk/Contents/Home/include/darwin
endif
# XXX: still needs gstreamer headers installed on OS X (e.g. with "brew install gstreamer")
#CFLAGS += $(shell pkg-config gstreamer-1.0 --cflags-only-I)
CFLAGS += -I/Library/Frameworks/GStreamer.framework/Headers


LDFLAGS := -shared
ifeq ($(PLATFORM),Linux)
	LDFLAGS += $(shell pkg-config gstreamer-1.0 --libs)
endif
ifeq ($(PLATFORM),Darwin)
#	LDFLAGS += -arch i386 -arch x86_64
	LDFLAGS += -arch x86_64
#	LDFLAGS += -L../../library/macosx
    LDFLAGS += -L/Library/Frameworks/GStreamer.framework/Libraries
	LDFLAGS += -lgstreamer-1.0.0 -lgstapp-1.0.0 -lglib-2.0.0 -lgobject-2.0.0
endif


$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

iface.h:
	javah -classpath .. -o iface.h processing.simplevideo.SimpleVideo

clean:
	rm -f $(TARGET) $(OBJS)
