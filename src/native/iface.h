/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class processing_simplevideo_SimpleVideo */

#ifndef _Included_processing_simplevideo_SimpleVideo
#define _Included_processing_simplevideo_SimpleVideo
#ifdef __cplusplus
extern "C" {
#endif
/*
 * Class:     processing_simplevideo_SimpleVideo
 * Method:    gstreamer_init
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_processing_simplevideo_SimpleVideo_gstreamer_1init
  (JNIEnv *, jclass);

/*
 * Class:     processing_simplevideo_SimpleVideo
 * Method:    gstreamer_loadFile
 * Signature: (Ljava/lang/String;Ljava/lang/String;)J
 */
JNIEXPORT jlong JNICALL Java_processing_simplevideo_SimpleVideo_gstreamer_1loadFile
  (JNIEnv *, jobject, jstring, jstring);

/*
 * Class:     processing_simplevideo_SimpleVideo
 * Method:    gstreamer_play
 * Signature: (JZ)V
 */
JNIEXPORT void JNICALL Java_processing_simplevideo_SimpleVideo_gstreamer_1play
  (JNIEnv *, jobject, jlong, jboolean);

/*
 * Class:     processing_simplevideo_SimpleVideo
 * Method:    gstreamer_seek
 * Signature: (JF)V
 */
JNIEXPORT void JNICALL Java_processing_simplevideo_SimpleVideo_gstreamer_1seek
  (JNIEnv *, jobject, jlong, jfloat);

/*
 * Class:     processing_simplevideo_SimpleVideo
 * Method:    gstreamer_set_loop
 * Signature: (JZ)V
 */
JNIEXPORT void JNICALL Java_processing_simplevideo_SimpleVideo_gstreamer_1set_1loop
  (JNIEnv *, jobject, jlong, jboolean);

/*
 * Class:     processing_simplevideo_SimpleVideo
 * Method:    gstreamer_get_duration
 * Signature: (J)F
 */
JNIEXPORT jfloat JNICALL Java_processing_simplevideo_SimpleVideo_gstreamer_1get_1duration
  (JNIEnv *, jobject, jlong);

/*
 * Class:     processing_simplevideo_SimpleVideo
 * Method:    gstreamer_get_time
 * Signature: (J)F
 */
JNIEXPORT jfloat JNICALL Java_processing_simplevideo_SimpleVideo_gstreamer_1get_1time
  (JNIEnv *, jobject, jlong);

/*
 * Class:     processing_simplevideo_SimpleVideo
 * Method:    gstreamer_get_frame
 * Signature: (J)[B
 */
JNIEXPORT jbyteArray JNICALL Java_processing_simplevideo_SimpleVideo_gstreamer_1get_1frame
  (JNIEnv *, jobject, jlong);

#ifdef __cplusplus
}
#endif
#endif
