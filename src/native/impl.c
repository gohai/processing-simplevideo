#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <jni.h>
#include <gst/gl/gl.h>	// part of libgstreamer-plugins-bad1.0-dev
#include <gst/gst.h>
#include <gst/app/gstappsink.h>   // for GstAppSink, part of gstreamer-plugins-base
#include "impl.h"
#include "iface.h"

// https://github.com/ystreet/gst-plugins-gl/blob/master/tests/examples/gtk/gstgtk.c
#if defined(GST_GL_HAVE_WINDOW_WIN32) && defined(GDK_WINDOWING_WIN32)
// #include <gdk/gdkwin32.h>
#endif

#if defined(GST_GL_HAVE_WINDOW_COCOA)
// #include <gdk/gdkquartz.h>
//   #include <gst/gl/cocoa/gstglcaopengllayer.h>
//   #include <gst/gl/cocoa/gstglcontext_cocoa.h>  
#endif

#if GST_GL_HAVE_WINDOW_WAYLAND && defined (GDK_WINDOWING_WAYLAND)
// #include <gdk/gdkwayland.h>
// #include <gst/gl/wayland/gstgldisplay_wayland.h>
#endif

#if defined(GST_GL_HAVE_WINDOW_X11) && defined(GDK_WINDOWING_X11)
// #include <gdk/gdkx.h>
// #include <gst/gl/x11/gstgldisplay_x11.h>
// #include <gst/gl/x11/gstglcontext_glx.h>
#endif



GThread *thread;
GMainLoop *loop;
unsigned long glctx;
unsigned long glwin;
guintptr ctx_ptr;

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
  // g_print("Got %s message\n", GST_MESSAGE_TYPE_NAME (message));

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
    case GST_MESSAGE_NEED_CONTEXT: {
      g_print("Got %s message\n", GST_MESSAGE_TYPE_NAME (message));
      
      const gchar *context_type;
      GstContext *context = NULL;
     
      gst_message_parse_context_type (message, &context_type);
      
      if (g_strcmp0 (context_type, "gst.gl.app_context") == 0) {
#if GST_GL_HAVE_OPENGL
  g_print("GST_GL_HAVE_OPENGL\n");
#endif
#if GST_GL_HAVE_OPENGL2
  g_print("GST_GL_HAVE_OPENGL2\n");
#endif
#if GST_GL_HAVE_OPENGL3
  g_print("GST_GL_HAVE_OPENGL3\n");
#endif
#if GST_GL_HAVE_OPENGL4
  g_print("GST_GL_HAVE_OPENGL4\n");
#endif
#if GST_GL_HAVE_GLES1
  g_print("GST_GL_API_GLES1\n");
#endif
#if GST_GL_HAVE_GLES2
  g_print("GST_GL_API_GLES2\n");
#endif

#if GST_GL_HAVE_PLATFORM_GLX
  g_print("GST_GL_HAVE_PLATFORM_GLX\n");
#endif
#if GST_GL_HAVE_PLATFORM_EGL
  g_print("GST_GL_HAVE_PLATFORM_EGL\n");
#endif
#if GST_GL_HAVE_PLATFORM_CGL
  g_print("GST_GL_HAVE_PLATFORM_CGL\n");
#endif
#if GST_GL_HAVE_PLATFORM_WGL
  g_print("GST_GL_HAVE_PLATFORM_WGL\n");
#endif
#if GST_GL_HAVE_PLATFORM_EAGL
  g_print("GST_GL_HAVE_PLATFORM_EAGL\n");
#endif
         
         g_print("setting %s\n", context_type);

         /* get this from the application somehow */
         GstGLContext *gl_context;         
         GstGLDisplay* display = gst_gl_display_new();
//          guintptr ctx_ptr = gst_gl_context_get_current_gl_context(GST_GL_PLATFORM_CGL);    

         
         gl_context = gst_gl_context_new_wrapped(display, ctx_ptr /*glctx*/, 
                                  GST_GL_PLATFORM_CGL, GST_GL_API_OPENGL);
       
//        g_print("HAHAHA %i\n", gst_gl_context_cocoa_get_current_context ());
       
//                  g_print("display %lu\n", display);         
//                  g_print("context %lu\n", ctx_ptr);   
//                  g_print("external context %lu\n", glctx);
                     
                 g_print("GstGLContext %lu\n", gl_context); 
                 
        
        GstStructure *s;

        context = gst_context_new (GST_GL_DISPLAY_CONTEXT_TYPE, TRUE);
        s = gst_context_writable_structure (context);
        gst_structure_set (s, "context", GST_GL_TYPE_CONTEXT, gl_context, NULL);

        gst_element_set_context (GST_ELEMENT (message->src), context);
      }
      if (context)
        gst_context_unref (context);
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


JNIEXPORT jlong JNICALL Java_processing_simplevideo_SimpleVideo_gstreamer_1loadFile(JNIEnv *env, jobject obj, jstring _fn, jstring _pipeline, jlong window, jlong context)
{
  GError *error = NULL;

  video *v = new_video();
  if (v == NULL) {
    return 0L;
  }

  GstGLPlatform platform;
  GstGLAPI gl_api;
  guintptr gl_handle;
//   #if GST_GL_HAVE_WINDOW_X11  
//   #endif
//   #if GST_GL_HAVE_WINDOW_WAYLAND
//   #endif
//   #if GST_GL_HAVE_WINDOW_COCOA
//   #endif

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

//   GstElement *glupload = gst_bin_get_by_name (GST_BIN (v->play), "glup");
//   g_print("glupload: %i\n", glupload);  
//   g_object_set (G_OBJECT (glupload), "external-opengl-context", context, NULL);
//   gst_object_unref (glupload);

    ctx_ptr = gst_gl_context_get_current_gl_context(GST_GL_PLATFORM_CGL);    
    g_print("gl context at init  %lu\n", ctx_ptr);
    glctx = context;
    glwin = window;
//     g_print("external context at init  %lu\n", context);
//     g_print("external window at init  %lu\n", window);
    g_print("************************************\n");
        
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
    
  video *v = (video*)user_data;
  if (v->buf[1] != 0) {
    v->buf[1] = 0;
  }
  // LOCK
  v->buf[1] = v->buf[0];
  v->buf[0] = texture;
  // UNLOCK

  gst_video_frame_unmap (&v_frame);

  gst_sample_unref(sample);

  return GST_FLOW_OK;
}


// Homepage of OpenGL Helper Library 
// http://gstreamer.freedesktop.org/data/doc/gstreamer/head/gst-plugins-bad-libs/html/gl.html

// Basic example with glimagesink
// http://cgit.freedesktop.org/gstreamer/gst-plugins-bad/tree/tests/examples/gl/generic/doublecube/main.cpp

// Context sharing example
// http://cgit.freedesktop.org/gstreamer/gst-plugins-bad/tree/tests/examples/gl/clutter/cluttershare.c

// use in Cinder (check how they handle sound)
// https://github.com/mikecreighton/cinder-GStreamer-Integration/blob/master/src/GstGLVideoPlayer.cpp


JNIEXPORT jint JNICALL Java_processing_simplevideo_SimpleVideo_gstreamer_1get_1frame
  (JNIEnv *env, jobject obj, jlong handle)
{
  video *v = get_video(handle);
  if (v == 0) {
    return 0L;
  }

  if (v->buf[0] == 0) {
    return 0L;
  }

  // LOCK
  jint ret = v->buf[0];
//   jbyteArray ret = (*env)->NewByteArray(env, v->buf[0]->size);
//   (*env)->SetByteArrayRegion(env, ret, 0, v->buf[0]->size, (const jbyte*)v->buf[0]->data);
  // UNLOCK
  return ret;
}
