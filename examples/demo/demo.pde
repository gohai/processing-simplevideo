import processing.simplevideo.SimpleVideo;
SimpleVideo video;

void setup() {
  size(640, 360);
  video = new SimpleVideo(this, "big-buck-bunny_trailer.webm");
  video.loop();
  frameRate(30);
}

void draw() {
  println(video.time() + " of " + video.duration());
  PImage frame = video.getFrame();
  if (frame != null) {
    image(frame, 0, 0);
  }
}

void keyPressed() {
  video.jump(random(0, video.duration()));
}

void movieEvent(int[] pixels) {
  println("movie event");
}
