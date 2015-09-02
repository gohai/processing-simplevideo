#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <jni.h>
#include <gst/gl/gl.h>
#include <gst/gst.h>
#include <gst/app/gstappsink.h>   // for GstAppSink, part of gstreamer-plugins-base
#include "impl.h"
#include "iface.h"

GThread *thread;
GMainLoop *loop;

#define MAX_VIDEOS 10
video videos[MAX_VIDEOS];


JNIEXPORT jboolean JNICALL Java_processing_simplevideo_SimpleVideo_gstreamer_1init(JNIEnv *env, jclass cls)
{
  GError *err = NULL;

  gst_init_check(NULL, NULL, &err);
  if (err != NULL) {
    g_print("Could not initialize library: %s\n", err->message);
    g_error_free(err);
    return FALSE;
  }

  thread = g_thread_new("simplevideo-mainloop", simplevideo_mainloop, NULL);
  return TRUE;
}


static void* simplevideo_mainloop(void *data) {
  loop = g_main_loop_new(NULL, FALSE);
  g_main_loop_run(loop);
  return NULL;
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


JNIEXPORT jlong JNICALL Java_processing_simplevideo_SimpleVideo_gstreamer_1loadFile(JNIEnv *env, jobject obj, jstring _fn, jstring _pipeline)
{
  GError *error = NULL;

  video *v = new_video();
  if (v == NULL) {
    return 0L;
  }

  // encode filename as an uri
  const char *fn = (*env)->GetStringUTFChars(env, _fn, JNI_FALSE);
  gchar *uri;
  if (strstr(fn, "://") == NULL) {
    uri = gst_filename_to_uri(fn, NULL);
  } else {
    uri = g_strdup(fn);
  }

  // create a new pipeline
  const char *pipeline = (*env)->GetStringUTFChars(env, _pipeline, JNI_FALSE);
  gchar *descr = g_strdup_printf(pipeline, uri);
  g_free(uri);
  (*env)->ReleaseStringUTFChars(env, _pipeline, pipeline);
  (*env)->ReleaseStringUTFChars(env, _fn, fn);

  v->play = gst_parse_launch(descr, &error);

  if (error != NULL) {
    g_print("Could not construct pipeline: %s\n", error->message);
    g_error_free(error);
    g_free(descr);
    return 0L;
  }

  // setup appsink if the pipeline  is using it
  if (strstr(descr, "appsink")) {
    setupAppsink(v);
  }
  g_free(descr);

  v->bus = gst_pipeline_get_bus(GST_PIPELINE (v->play));
  gst_bus_add_watch(v->bus, simplevideo_bus_callback, loop);
  gst_object_unref(v->bus);

  v->obj = obj;

  return (jlong)v;
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


void setupAppsink(video *v)
{
  // get sink
  // set http://gstreamer.freedesktop.org/data/doc/gstreamer/head/gst-plugins-base-libs/html/gst-plugins-base-libs-appsink.html
  // XXX: use gst_bin_get_by_interface
  v->sink = gst_bin_get_by_name(GST_BIN (v->play), "sink");
  gst_app_sink_set_max_buffers(GST_APP_SINK(v->sink), 2); // limit number of buffers queued
  gst_app_sink_set_drop(GST_APP_SINK(v->sink), TRUE );    // drop old buffers in queue when full

  // setup callbacks (faster than signals)
  GstAppSinkCallbacks* appsink_callbacks = (GstAppSinkCallbacks*)malloc(sizeof(GstAppSinkCallbacks));
  appsink_callbacks->eos = NULL;
  appsink_callbacks->new_preroll = NULL;
  appsink_callbacks->new_sample = appsink_new_sample;
  gst_app_sink_set_callbacks(GST_APP_SINK(v->sink), appsink_callbacks, v, NULL);
  free(appsink_callbacks);

//    g_signal_connect(G_OBJECT(v->sink), "client-draw", G_CALLBACK (drawCallback), NULL);
}


//client draw callback
// This might be useful:
// https://github.com/mikecreighton/cinder-GStreamer-Integration/blob/master/src/GstGLVideoPlayer.cpp
static gboolean drawCallback (GstElement * gl_sink, GstGLContext *context, GstSample * sample, gpointer data)
{
    static GLfloat	xrot = 0;
    static GLfloat	yrot = 0;
    static GLfloat	zrot = 0;
    static GTimeVal current_time;
//     static glong last_sec = current_time.tv_sec;
//     static gint nbFrames = 0;

    GstVideoFrame v_frame;
    GstVideoInfo v_info;
    guint texture = 0;
    GstBuffer *buf = gst_sample_get_buffer (sample);
    GstCaps *caps = gst_sample_get_caps (sample);

    gst_video_info_from_caps (&v_info, caps);    
    if (!gst_video_frame_map (&v_frame, &v_info, buf, (GstMapFlags) (GST_MAP_READ | GST_MAP_GL))) {
      g_warning ("Failed to map the video buffer");
      return TRUE;
    }    
    
    texture = *(guint *) v_frame.data[0];

//     g_get_current_time (&current_time);
//     nbFrames++ ;    
    
    
    gst_video_frame_unmap (&v_frame);
    
    g_print("draw: %i\n", texture);

    return TRUE;
}





// see http://stackoverflow.com/questions/28040857/gstreamer-write-appsink-to-filesink
// see http://stackoverflow.com/questions/24142381/probleme-with-the-pull-sample-signal-using-appsink
// see http://gstreamer.freedesktop.org/data/doc/gstreamer/head/gst-plugins-base-libs/html/gst-plugins-base-libs-appsink.html
static GstFlowReturn appsink_new_sample(GstAppSink *sink, gpointer user_data)
{
  // prog_data* pd = (prog_data*)user_data;
  GstSample* sample = gst_app_sink_pull_sample(sink);

  if(sample == NULL) {
    return GST_FLOW_ERROR;
  }

  GstBuffer* buffer = gst_sample_get_buffer(sample);


    GstVideoFrame v_frame;
    GstVideoInfo v_info;
    guint texture = 0;
    GstBuffer *buf = gst_sample_get_buffer (sample);
    GstCaps *caps = gst_sample_get_caps (sample);

    gst_video_info_from_caps (&v_info, caps);    
    if (!gst_video_frame_map (&v_frame, &v_info, buf, (GstMapFlags) (GST_MAP_READ | GST_MAP_GL))) {
      g_warning ("Failed to map the video buffer");
      return TRUE;
    }    
    
    texture = *(guint *) v_frame.data[0];
    
  g_print("draw: %i\n", texture);

//   GstMemory* memory = gst_buffer_get_all_memory(buffer);
//   GstMapInfo* map_info = malloc(sizeof(GstMapInfo));
// 
//   if(!gst_memory_map(memory, map_info, GST_MAP_READ)) {
//     gst_memory_unref(memory);
//     gst_sample_unref(sample);
//     return GST_FLOW_ERROR;
//   }
// 
//   video *v = (video*)user_data;
//   if (v->buf[1] != NULL) {
//     // free previous sample
//     GstMapInfo *old_map_info = v->buf[1];
//     GstMemory *old_memory = old_map_info->memory;
//     gst_memory_unmap(old_memory, old_map_info);
//     gst_memory_unref(old_memory);
//     free(old_map_info);
//     v->buf[1] = NULL;
//   }
//   // LOCK
//   v->buf[1] = v->buf[0];
//   v->buf[0] = map_info;
//   // UNLOCK

  gst_video_frame_unmap (&v_frame);

  //gst_memory_unmap(memory, &map_info);
  //gst_memory_unref(memory);
  gst_sample_unref(sample);

  return GST_FLOW_OK;
}


// Idea: Use gst-gl element?
// https://coaxion.net/blog/2014/04/opengl-support-in-gstreamer/
// http://cgit.freedesktop.org/gstreamer/gst-plugins-bad/tree/tests/examples/gl/generic/doublecube/main.cpp


JNIEXPORT jbyteArray JNICALL Java_processing_simplevideo_SimpleVideo_gstreamer_1get_1frame
  (JNIEnv *env, jobject obj, jlong handle)
{
  video *v = get_video(handle);
  if (v == NULL) {
    return 0L;
  }

  if (v->buf[0] == NULL) {
    return 0L;
  }

  // LOCK
  jbyteArray ret = (*env)->NewByteArray(env, v->buf[0]->size);
  (*env)->SetByteArrayRegion(env, ret, 0, v->buf[0]->size, (const jbyte*)v->buf[0]->data);
  // UNLOCK
  return ret;
}
