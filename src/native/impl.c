#include <stdio.h>
#include <string.h>
#include <jni.h>
#include <gst/gst.h>
#include <gst/app/app.h>
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

// http://stackoverflow.com/questions/28040857/gstreamer-write-appsink-to-filesink
// http://stackoverflow.com/questions/24142381/probleme-with-the-pull-sample-signal-using-appsink
// http://gstreamer.freedesktop.org/data/doc/gstreamer/head/gst-plugins-base-libs/html/gst-plugins-base-libs-appsink.html
static GstFlowReturn app_sink_new_sample(GstAppSink *sink, gpointer user_data) {
//   prog_data* pd = (prog_data*)user_data;

  GstSample* sample = gst_app_sink_pull_sample(sink);
  
  if(sample == NULL) {
    return GST_FLOW_ERROR;
  }

  GstBuffer* buffer = gst_sample_get_buffer(sample);

  GstMemory* memory = gst_buffer_get_all_memory(buffer);
  GstMapInfo map_info;

  if(! gst_memory_map(memory, &map_info, GST_MAP_READ)) {
    gst_memory_unref(memory);
    gst_sample_unref(sample);
    return GST_FLOW_ERROR;
  }

  //render using map_info.data
   callback(map_info);

  gst_memory_unmap(memory, &map_info);
  gst_memory_unref(memory);
  gst_sample_unref(sample);

  g_print ("read sample!\n");

  return GST_FLOW_OK;
}


// Some general JNI references:
// http://docs.oracle.com/javase/7/docs/technotes/guides/jni/
// http://www.math.uni-hamburg.de/doc/java/tutorial/native1.1/implementing/method.html
// http://www.ibm.com/developerworks/library/j-jni/


// cached refs for later callbacks
JavaVM *g_vm;
jobject g_obj;
jmethodID g_mid;
JNIEnv *g_env_ch;

// Following technique described in:
// http://adamish.com/blog/archives/327
// First we need to register the callback method in java, and save the references.
// Second, the callback needs to attach the current thread of the JVM because gstreamer
// runs on a separate thread (I believe).
JNIEXPORT jboolean JNICALL Java_processing_simplevideo_SimpleVideo_gstreamer_1register
	(JNIEnv *env, jobject obj) {
        int returnValue = TRUE;
		// convert local to global reference 
        // (local will die after this method call)
		g_obj = (*env)->NewGlobalRef(env, obj);
        g_env_ch = env;
        
		// save refs for callback
		jclass g_clazz = (*env)->GetObjectClass(env, g_obj);
		if (g_clazz == NULL) {
          g_print ("Failed to find class\n");			
		}

		g_mid = (*env)->GetMethodID(env, g_clazz, "readFrame", "([I)V");
		if (g_mid == NULL) {
          g_print ("Unable to get method ref\n");
		}
		
        // Acquire a pointer to the current JavaVM 
        jsize jvmBufferLength = 1;                 // At most vmBufLength number of entries  will be written for the list of returned JavaVMs 
        jsize jvmTotalNumberFound = 0;          // The total number of JavaVMs found 
        JavaVM jvmBuffer[jvmBufferLength];        // Array of JavaVMs 
        g_vm = jvmBuffer;         // Pointer to array of JavaVMs 
        
        // Problem:
        // http://stackoverflow.com/questions/7118750/failed-to-locate-method-jni-getcreatedjavavms-in-the-libjvm-dylib-mac-os
        // Solved by adding "-framework JavaVM" to LD flags
        // http://octave.1599824.n4.nabble.com/java-package-and-MacOS-td4647144.html
        jint result  = JNI_GetCreatedJavaVMs( &g_vm, jvmBufferLength,  &jvmTotalNumberFound); // Get all created JavaVMs 
        g_print ("Number of JVMs found %i\n", jvmTotalNumberFound);
        if (jvmTotalNumberFound < 1) {
          g_print ("Unable to get JVM ref\n");
          returnValue = FALSE;
        }

		return (jboolean)returnValue;
}


static void callback(GstMapInfo map_info) {

// calling from the cached environment crashes the java application
//  (*g_env)->CallVoidMethod(g_obj, g_mid, val);

	JNIEnv *g_env;
	// double check it's all ok

	int getEnvStat = (*g_vm)->GetEnv(g_vm, (void **)&g_env, JNI_VERSION_1_6);
	if (getEnvStat == JNI_EDETACHED) {
		g_print ("GetEnv: not attached\n");
		if ((*g_vm)->AttachCurrentThread(g_vm, (void **) &g_env, NULL) != 0) {
			g_print ("Failed to attach\n");
		} else {
		  g_print ("Attached successfully!\n");
		}
	} else if (getEnvStat == JNI_OK) {
		g_print ("Already attached!\n");
	} else if (getEnvStat == JNI_EVERSION) {
		g_print ("GetEnv: version not supported\n");
	} else {
      g_print ("Unknown status %i\n", getEnvStat);
    }

//  https://developer.gnome.org/gstreamer/stable/gstreamer-GstMemory.html#GstMapInfo
   gsize size = map_info.size;

  // Some faster methods: use a direct byte buffer:
  // http://stackoverflow.com/questions/15339430/wrap-native-int-into-a-jintarray
  // but what happens when the sample is unreferenced?

    // The slowest possible way
   jintArray pixels = (*g_env)->NewIntArray(g_env, size);
   jint *body = (*g_env)->GetIntArrayElements(g_env, pixels, 0); // needs this, otherwise SetIntArrayRegion crashes the application
   (*g_env)->SetIntArrayRegion(g_env, pixels, 0 , size, map_info.data);


	(*g_env)->CallVoidMethod(g_env, g_obj, g_mid, pixels);	
	

	if ((*g_env)->ExceptionCheck(g_env)) {
		(*g_env)->ExceptionDescribe(g_env);
	}

	(*g_vm)->DetachCurrentThread(g_vm);
	
//     g_print ("buffer size %i\n", val);
	
}






// Idea: Use gst-gl element?
// https://coaxion.net/blog/2014/04/opengl-support-in-gstreamer/
// http://cgit.freedesktop.org/gstreamer/gst-plugins-bad/tree/tests/examples/gl/generic/doublecube/main.cpp


#define CAPS "video/x-raw,format=RGB,width=640,height=360,pixel-aspect-ratio=1/1"
JNIEXPORT jlong JNICALL Java_processing_simplevideo_SimpleVideo_gstreamer_1loadFile(JNIEnv *env, jobject obj, jstring _fn)
{
    GError *error = NULL;

	video *v = new_video();
	if (v == NULL) {
		return 0L;
	}

	const char *fn = (*env)->GetStringUTFChars(env, _fn, JNI_FALSE);
	gchar *uri;
	if (strstr(fn, "://") == NULL) {
		uri = gst_filename_to_uri(fn, NULL);
	} else {
		uri = g_strdup(fn);
	}
	
    /* create a new pipeline */
    gchar *descr = g_strdup_printf ("uridecodebin uri=%s ! videoconvert ! videoscale ! "
                                    " appsink name=sink caps=\"" CAPS "\"", uri);                                     
    g_free(uri);
    (*env)->ReleaseStringUTFChars(env, _fn, fn);
    
    v->play = gst_parse_launch (descr, &error);

    if (error != NULL) {
      g_print ("could not construct pipeline: %s\n", error->message);
      g_error_free (error);
      exit (-1);
    }

  
    /* set to PAUSED to make the first frame arrive in the sink */
    GstStateChangeReturn ret = gst_element_set_state (v->play, GST_STATE_PAUSED);
    switch (ret) {
      case GST_STATE_CHANGE_FAILURE:
        g_print ("failed to play the file\n");
        exit (-1);
      case GST_STATE_CHANGE_NO_PREROLL:
        /* for live sources, we need to set the pipeline to PLAYING before we can
         * receive a buffer. We don't do that yet */
        g_print ("live sources not supported yet\n");
        exit (-1);
      default:
        break;
    }
    /* This can block for up to 5 seconds. If your machine is really overloaded,
     * it might time out before the pipeline prerolled and we generate an error. A
     * better way is to run a mainloop and catch errors there. */
    ret = gst_element_get_state (v->play, NULL, NULL, 5 * GST_SECOND);
    if (ret == GST_STATE_CHANGE_FAILURE) {
      g_print ("failed to play the file\n");
      exit (-1);
    }  
    
    /* get the duration */
    gint64 duration;
    gst_element_query_duration (v->play, GST_FORMAT_TIME, &duration);  
  
    //g_signal_connect(G_OBJECT(v->sink), "new-sample", G_CALLBACK (drawCallback), NULL);

    /* get sink */
//     http://gstreamer.freedesktop.org/data/doc/gstreamer/head/gst-plugins-base-libs/html/gst-plugins-base-libs-appsink.html
    v->sink = gst_bin_get_by_name (GST_BIN (v->play), "sink");  
    gst_app_sink_set_max_buffers (GST_APP_SINK(v->sink), 2); // limit number of buffers queued
    gst_app_sink_set_drop(GST_APP_SINK(v->sink), TRUE ); // drop old buffers in queue when full

// Using callbacks
   GstAppSinkCallbacks* appsink_callbacks = (GstAppSinkCallbacks*)malloc(sizeof(GstAppSinkCallbacks));
   appsink_callbacks->eos = NULL;
   appsink_callbacks->new_preroll = NULL;
   appsink_callbacks->new_sample = app_sink_new_sample;
   gst_app_sink_set_callbacks(GST_APP_SINK(v->sink), appsink_callbacks, NULL, NULL);

// Using signals
//     gst_app_sink_set_emit_signals(GST_APP_SINK(v->sink), TRUE ); // Using signals
//     g_signal_connect(G_OBJECT(v->sink), "new-sample", G_CALLBACK (app_sink_new_sample), NULL);


	v->bus = gst_pipeline_get_bus(GST_PIPELINE (v->play));
	gst_bus_add_watch(v->bus, simplevideo_bus_callback, loop);
	gst_object_unref(v->bus);
	
	v->obj = obj;

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
