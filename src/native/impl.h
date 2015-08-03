#ifndef IMPL_H
#define IMPL_H

typedef struct video_type {
	GstElement *play;
	GstBus *bus;
	jobject obj;
	int loop;
} video;

static void* simplevideo_mainloop(void *data);
static gboolean simplevideo_bus_callback(GstBus *bus, GstMessage *message, gpointer data);
static video* new_video();
static video* get_video(long handle);

#endif