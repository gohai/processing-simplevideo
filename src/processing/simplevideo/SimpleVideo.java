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
import java.lang.reflect.*;
import processing.core.*;

public class SimpleVideo {

  protected static boolean loaded = false;
  protected static boolean error = false;

  // capabilities passed to GStreamer
  protected static String caps = "video/x-raw,format=RGB,width=640,height=360,pixel-aspect-ratio=1/1";

  // pipeline description passed to GStreamer
  // first %s is the uri (filled in by native code)
  // appsink must be named "sink" (XXX: change)
  private static String pipeline = "uridecodebin uri=%s ! videoconvert ! videoscale ! appsink name=sink caps=\"" + caps + "\"";
  // alternatively, a standalone window
  //private static String pipeline = "playbin uri=%s";

  private PApplet parent;
  private Method movieEventMethod;
  private long handle = 0;

  public SimpleVideo(PApplet parent, String fn) {
    super();
    this.parent = parent;

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

    PImage frame = parent.createImage(640, 360, PConstants.RGB);

    // XXX: we also need to handle the audio somehow
    int idx = 0;
    frame.loadPixels();
    for (int i = 0; i < buffer.length/3; i++) {
      int ri = 3 * i + 0;
      int gi = 3 * i + 1;
      int bi = 3 * i + 2;
      int r = buffer[ri] & 0xff;
      int g = buffer[gi] & 0xff;
      int b = buffer[bi] & 0xff;
      frame.pixels[idx++] = 0xFF000000 | (r << 16) | (g << 8) | b;
    }
    frame.updatePixels();

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
