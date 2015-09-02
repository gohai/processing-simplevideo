/* -*- mode: java; c-basic-offset: 2; indent-tabs-mode: nil -*- */

/*
  Copyright (c) The Processing Foundation 2015
  Developed by Gottfried Haider, as part of GSOC 2015, & Andres Colubri

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General
  Public License along with this library; if not, write to the
  Free Software Foundation, Inc., 59 Temple Place, Suite 330,
  Boston, MA  02111-1307  USA
*/

package processing.simplevideo;

import java.io.File;
import java.nio.*;
import java.lang.reflect.*;
import processing.core.*;

public class SimpleVideo {

  protected static boolean loaded = false;
  protected static boolean error = false;


  private PApplet parent;
  private Method movieEventMethod;
  private long handle = 0;
  private PImage frame;

  public SimpleVideo(PApplet parent, String fn) {
    super();
    this.parent = parent;

    int w = parent.width;
    int h = parent.height;
 
    // capabilities passed to GStreamer
    String caps = "video/x-raw,format=ARGB,width=" + w + ",height=" + h + ",pixel-aspect-ratio=1/1";

    // pipeline description passed to GStreamer
    // first %s is the uri (filled in by native code)
    // appsink must be named "sink" (XXX: change)
    String pipeline = "uridecodebin uri=%s ! videoconvert ! video/x-raw,format=ARGB ! videoscale ! appsink name=sink caps=\"" + caps + "\"";
    // alternatively, a standalone window
    //private static String pipeline = "playbin uri=%s";


    if (!loaded) {
      System.loadLibrary("simplevideo");
      loaded = true;
      if (gstreamer_init() == false) {
        error = true;
      }
    }

    if (error) {
      throw new RuntimeException("Could not load gstreamer");
    }

    // get absolute path for fn
    if (fn.indexOf("://") != -1) {
      // got URI, use as is
    } else {
      // first, check Processing's dataPath
      File file = new File(parent.dataPath(fn));
      if (file.exists() == false) {
        // next, the current directory
        file = new File(fn);
      }
      if (file.exists()) {
        fn = file.getAbsolutePath();
      }
    }

    handle = gstreamer_loadFile(fn, pipeline);
    if (handle == 0) {
      throw new RuntimeException("Could not load video");
    }

    try {
      movieEventMethod = parent.getClass().getMethod("movieEvent", int[].class);
    } catch (Exception e) {
      // no event method declared, ignore
    }
    frame = parent.createImage(parent.width, parent.height, PConstants.RGB);
    frame.loadPixels();
  }

  public void dispose() {
    // XXX: not working?
    System.out.println("called dispose");
  }

  public void pre() {
    // XXX: not working?
    System.out.println("called pre");
  }

  public void post() {
    // XXX: not working
    System.out.println("called post");
  }

  public void play() {
    gstreamer_play(handle, true);
  }

  public void pause() {
    gstreamer_play(handle, false);
  }

  public void stop() {
    gstreamer_play(handle, false);
    gstreamer_seek(handle, 0.0f);
  }

  public void loop() {
    gstreamer_set_loop(handle, true);
    gstreamer_play(handle, true);
  }

  public void noLoop() {
    gstreamer_set_loop(handle, false);
  }

  public void jump(float sec) {
    gstreamer_seek(handle, sec);
  }

  public float duration() {
    return gstreamer_get_duration(handle);
  }

  public float time() {
    return gstreamer_get_time(handle);
  }

  public PImage getFrame() {
    byte[] buffer = gstreamer_get_frame(handle);
    if (buffer == null) {
      return null;
    }
//     PApplet.println(buffer.length/4 + " " + frame.pixels.length);


    final ByteBuffer buf = ByteBuffer.wrap(buffer).order(ByteOrder.BIG_ENDIAN);
    buf.asIntBuffer().get(frame.pixels);
    frame.updatePixels();

    // XXX: we also need to handle the audio somehow
//     int idx = 0;
//     for (int i = 0; i < buffer.length/4; i++) {
//       int ri = 4 * i + 1;
//       int gi = 4 * i + 2;
//       int bi = 4 * i + 3;
//       int r = buffer[ri] & 0xff;
//       int g = buffer[gi] & 0xff;
//       int b = buffer[bi] & 0xff;
//       frame.pixels[idx++] = 0xFF000000 | (r << 16) | (g << 8) | b;
//     }
//     frame.updatePixels();

    return frame;
  }

  /*
  if (movieEventMethod != null) {
    try {
      movieEventMethod.invoke(parent, pixels);
    } catch (Exception e) {
      e.printStackTrace();
      movieEventMethod = null;
    }
  }
  */

  private static native boolean gstreamer_init();
  private native long gstreamer_loadFile(String fn, String pipeline);
  private native void gstreamer_play(long handle, boolean play);
  private native void gstreamer_seek(long handle, float sec);
  private native void gstreamer_set_loop(long handle, boolean loop);
  private native float gstreamer_get_duration(long handle);
  private native float gstreamer_get_time(long handle);
  private native byte[] gstreamer_get_frame(long handle);
}
