#include <stdio.h>
#include <string.h>
#include <jni.h>
#include <gst/gst.h>
#include "impl.h"
#include "iface.h"

GThread *thread;
GMainLoop *loop;

#define MAX_VIDEOS 10
video videos[MAX_VIDEOS];

JNIEXPORT jboolean JNICALL Java_processing_simplevideo_SimpleVideo_gstreamer_1init (JNIEnv *env, jclass cls)
{
	GError *err = NULL;

	gst_init_check(NULL, NULL, &err);
	if (err != NULL) {
		// XXX: return err
		g_error_free(err);
		return FALSE;
	}

	thread = g_thread_new("simplevideo-mainloop", simplevideo_mainloop, NULL);
	return TRUE;
}

static void* simplevideo_mainloop(void *data) {
	loop = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(loop);
}

static gboolean simplevideo_bus_callback(GstBus *bus, GstMessage *message, gpointer data)
{
	//g_print("Got %s message\n", GST_MESSAGE_TYPE_NAME (message));

	switch (GST_MESSAGE_TYPE (message)) {
		case GST_MESSAGE_ERROR: {
			GError *err;
			gchar *debug;
			gst_message_parse_error(message, &err, &debug);
			g_print("Error: %s\n", err->message);
			g_error_free(err);
			g_free(debug);
			g_main_loop_quit(loop);
			break;
		}
		case GST_MESSAGE_EOS: {
			video *v = NULL;
			for (int i=0; i < MAX_VIDEOS; i++) {
				if (bus == videos[i].bus) {
					v = &videos[i];
					break;
				}
			}
			if (v && v->loop) {
				gst_element_seek(v->play, 1.0, GST_FORMAT_TIME, 
					GST_SEEK_FLAG_FLUSH, GST_SEEK_TYPE_SET, 0,
					GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE);
			}
			//g_main_loop_quit(loop);
			break;
		}
		default:
			break;
	}

	return TRUE;
}

static video* new_video()
{
	for (int i=0; i < MAX_VIDEOS; i++) {
		if (videos[i].obj == NULL) {
			return &videos[i];
		}
	}
	return NULL;
}

static video* get_video(long handle)
{
	for (int i=0; i < MAX_VIDEOS; i++) {
		if ((long)&videos[i] == handle) {
			return &videos[i];
		}
	}
	return NULL;
}

JNIEXPORT jlong JNICALL Java_processing_simplevideo_SimpleVideo_gstreamer_1loadFile(JNIEnv *env, jobject obj, jstring _fn)
{
	video *v = new_video();
	if (v == NULL) {
		return 0L;
	}

	const char *fn = (*env)->GetStringUTFChars(env, _fn, JNI_FALSE);
	v->play = gst_element_factory_make("playbin", "play");
	gchar *uri;
	if (strstr(fn, "://") == NULL) {
		uri = gst_filename_to_uri(fn, NULL);
	} else {
		uri = g_strdup(fn);
	}
	g_object_set(G_OBJECT (v->play), "uri", uri, NULL);
	g_free(uri);
	(*env)->ReleaseStringUTFChars(env, _fn, fn);

	v->bus = gst_pipeline_get_bus(GST_PIPELINE (v->play));
	gst_bus_add_watch(v->bus, simplevideo_bus_callback, loop);
	gst_object_unref(v->bus);

	v->obj = obj;

	//gst_object_unref(GST_OBJECT (play));

	// XXX: vs jlong?
	return (long)v;
}

JNIEXPORT void JNICALL Java_processing_simplevideo_SimpleVideo_gstreamer_1play(JNIEnv *env, jobject obj, jlong handle, jboolean _play)
{
	video *v = get_video(handle);
	if (v == NULL) {
		return;
	}

	if (_play == JNI_TRUE) {
		gst_element_set_state (v->play, GST_STATE_PLAYING);
	} else {
		gst_element_set_state (v->play, GST_STATE_PAUSED);
	}
}

JNIEXPORT void JNICALL Java_processing_simplevideo_SimpleVideo_gstreamer_1seek(JNIEnv *env, jobject obj, jlong handle, jfloat sec)
{
	video *v = get_video(handle);
	if (v == NULL) {
		return;
	}

	gst_element_seek (v->play, 1.0, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH|GST_SEEK_FLAG_KEY_UNIT,
		GST_SEEK_TYPE_SET, (gint64)(sec * 1000000000),
		GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE);
}

JNIEXPORT void JNICALL Java_processing_simplevideo_SimpleVideo_gstreamer_1set_1loop(JNIEnv *env, jobject obj, jlong handle, jboolean loop)
{
	video *v = get_video(handle);
	if (v == NULL) {
		return;
	}

	v->loop = loop;
}

JNIEXPORT jfloat JNICALL Java_processing_simplevideo_SimpleVideo_gstreamer_1get_1duration(JNIEnv *env, jobject obj, jlong handle)
{
	video *v = get_video(handle);
	if (v == NULL) {
		return -1.0f;
	}

	gint64 len;
	if (gst_element_query_duration(v->play, GST_FORMAT_TIME, &len)) {
		return len/1000000000.0f;
	} else {
		return -1.0f;
	}
}

JNIEXPORT jfloat JNICALL Java_processing_simplevideo_SimpleVideo_gstreamer_1get_1time(JNIEnv *env, jobject obj, jlong handle)
{
	video *v = get_video(handle);
	if (v == NULL) {
		return -1.0f;
	}

	gint64 pos;
	if (gst_element_query_position(v->play, GST_FORMAT_TIME, &pos)) {
		return pos/1000000000.0f;
	} else {
		return -1.0f;
	}
}
