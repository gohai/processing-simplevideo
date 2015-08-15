#ifndef IMPL_H
#define IMPL_H

typedef struct video_type {
	GstElement *play;
	GstElement *sink;
	GstBus *bus;
	jobject obj;
	int loop;
	GstMapInfo* buf[2];
} video;

static void* simplevideo_mainloop(void *data);
static GstFlowReturn appsink_new_sample(GstAppSink *sink, gpointer user_data);
static gboolean simplevideo_bus_callback(GstBus *bus, GstMessage *message, gpointer data);
static video* new_video();
static video* get_video(long handle);
static void callback(GstMapInfo map_info);
static void setupAppsink(video *v);

#endif