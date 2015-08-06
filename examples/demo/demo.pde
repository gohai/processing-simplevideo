import processing.simplevideo.SimpleVideo;
SimpleVideo video;
PImage frame;

void setup() {
  size(640, 360);
  video = new SimpleVideo(this, "transit.mov");
  video.loop();
  frameRate(30);
}

void draw() {
  println(video.time() + " of " + video.duration());
  if (frame != null) {
    image(frame, 0, 0);
  }
}

void keyPressed() {
  video.jump(random(0, video.duration()));
}

void movieEvent(int[] pixels) {
  println("movie event " + pixels.length);
  if (frame == null) {
    frame = createImage(640, 360, RGB);
  }
  int idx = 0;
  frame.loadPixels();
  for (int i = 0; i < pixels.length/3; i++) {
    int ri = 3 * i + 0;
    int gi = 3 * i + 1;
    int bi = 3 * i + 2;
    int r = pixels[ri];
    int g = pixels[gi];
    int b = pixels[bi];
    frame.pixels[idx++] = 0xFF000000 | (r << 16) | (g << 8) | b;
  }
  frame.updatePixels();
  
}