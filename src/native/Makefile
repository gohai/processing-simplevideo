TARGET := libsimplevideo.so
OBJS := impl.o
CC := gcc

CFLAGS := -fPIC -g
CFLAGS += -I/usr/java/latest/include
CFLAGS += -I/usr/java/latest/include/linux
CFLAGS += $(shell pkg-config gstreamer-1.0 --cflags-only-I)
LDFLAGS := -shared
LDFLAGS += $(shell pkg-config gstreamer-1.0 --libs)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

iface.h:
	javah -classpath .. -o iface.h processing.simplevideo.SimpleVideo

clean:
	rm -f $(TARGET) $(OBJS)
	rm -f iface.h
	