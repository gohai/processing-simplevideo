import processing.simplevideo.SimpleVideo;
SimpleVideo video;

void setup() {
  size(640, 360, P2D);
  video = new SimpleVideo(this, "big-buck-bunny_trailer.webm");
  
  //size(1920, 1038, P2D);
  //video = new SimpleVideo(this, "hd.mp4");
    
  //size(4096, 1716, P2D);
  //video = new SimpleVideo(this, "hd4k.mp4"); 
  
  
  
  video.loop();
  frameRate(60);
}

void draw() {
  background(0);
  //println(video.time() + " of " + video.duration());

  int tex = video.getFrame();
  System.out.println("texture " + tex);
  PGL pgl = beginPGL();
  pgl.drawTexture(PGL.TEXTURE_2D, tex, width, height, 
                  0, 0, width, height);
  endPGL();
    

  //PImage frame = video.getFrame();
  //if (frame != null) {
  //  image(frame, 0, 0, width, height);
  //}
  
  
  
  text(frameRate, 20, 20);
}

void keyPressed() {
  video.jump(random(0, video.duration()));
}

void movieEvent(int[] pixels) {
  println("movie event");
}