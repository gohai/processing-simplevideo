TARGET := libsimplevideo.so
OBJS := impl.o
CC := gcc
PLATFORM := $(shell uname -s)


CFLAGS := -std=c99 -fPIC -g
ifeq ($(PLATFORM),Linux)
	# XXX: derive from javac location
	CFLAGS += -I/usr/java/latest/include
	CFLAGS += -I/usr/java/latest/include/linux
	# also needs gstreamer1-plugins-base-devel
	CFLAGS += $(shell pkg-config gstreamer-1.0 --cflags-only-I)
endif
ifeq ($(PLATFORM),Darwin)
	# build multiarch
	CFLAGS += -arch i386 -arch x86_64
	# build 64 bit only
	#CFLAGS += -arch x86_64
	CFLAGS += -I/Library/Java/JavaVirtualMachines/jdk1.8.0_51.jdk/Contents/Home/include
	CFLAGS += -I/Library/Java/JavaVirtualMachines/jdk1.8.0_51.jdk/Contents/Home/include/darwin
	CFLAGS += -I/Library/Frameworks/GStreamer.framework/Headers
	CFLAGS += -I/Library/Frameworks/Gstreamer.framework/Versions/1.0/Headers
endif

LDFLAGS := -shared
ifeq ($(PLATFORM),Linux)
	LDFLAGS += $(shell pkg-config gstreamer-1.0 --libs)
	LDFLAGS += $(shell pkg-config gstreamer-app-1.0 --libs)
endif
ifeq ($(PLATFORM),Darwin)
	# build multiarch
	LDFLAGS += -arch i386 -arch x86_64
	# build 64 bit only
	#LDFLAGS += -arch x86_64
    #LDFLAGS += -L/Library/Frameworks/GStreamer.framework/Libraries
    LDFLAGS += -L/Library/Frameworks/Gstreamer.framework/Versions/1.0/lib
	LDFLAGS += -lgstreamer-1.0 -lgstapp-1.0 -lglib-2.0 -lgobject-2.0
endif

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

iface.h:
	javah -classpath .. -o iface.h processing.simplevideo.SimpleVideo

clean:
	rm -f $(TARGET) $(OBJS)
