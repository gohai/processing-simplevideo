import processing.simplevideo.SimpleVideo;
SimpleVideo video;

void setup() {
  video = new SimpleVideo(this, "transit.mov");
  video.loop();
  frameRate(10);
}

void draw() {
  println(video.time() + " of " + video.duration());
}

void keyPressed() {
  video.jump(random(0, video.duration()));
}