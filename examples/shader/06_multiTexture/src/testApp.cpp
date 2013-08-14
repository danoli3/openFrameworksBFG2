#include "testApp.h"

//--------------------------------------------------------------
void testApp::setup(){

#ifdef TARGET_OPENGLES
	shader.load("shadersES2/shader");
#else
	if(ofIsGLProgrammableRenderer()){
		shader.load("shadersGL3/shader");
	}else{
		shader.load("shadersGL2/shader");
	}
#endif
    
    int camWidth = 320;
	int camHeight = 240;
    
    camera.setVerbose(false);
	camera.initGrabber(camWidth, camHeight);
    
    movie.loadMovie("movie.mov");
	movie.play();
    
    image.loadImage("img.jpg");
    imageMask.loadImage("mask.jpg");
    
    fbo.allocate(camWidth, camHeight);
    maskFbo.allocate(camWidth, camHeight);
}

//--------------------------------------------------------------
void testApp::update(){
    camera.update();
    movie.update();
}

//--------------------------------------------------------------
void testApp::draw(){
    
    //------------------------------------------- draw to mask fbo.
    maskFbo.begin();

    ofClear(255, 0, 0, 255);
    
    float imageMaskX = mouseX / (float)ofGetWidth();
    imageMaskX = ofClamp(imageMaskX, 0, 1);
    imageMaskX = -(imageMask.getWidth() - maskFbo.getWidth()) * imageMaskX;
    imageMask.draw(imageMaskX, 0);
    
    maskFbo.end();
    
    //------------------------------------------- draw to final fbo.
    fbo.begin();
    ofClear(0, 0, 0,255);
    
    shader.begin();
    shader.setUniformTexture("tex0", camera.getTextureReference(), 1);
    shader.setUniformTexture("tex1", image, 2);
    shader.setUniformTexture("tex2", movie.getTextureReference(), 3);
    shader.setUniformTexture("imageMask", maskFbo.getTextureReference(), 4);
    
    // we are drawing this fbo so it is used just as a frame.
    maskFbo.draw(0, 0);
    
    shader.end();
    fbo.end();
    
    //------------------------------------------- 
    ofSetColor(255);
    camera.draw(5,5,320,240);
    ofSetColor(ofColor::red);
    ofDrawBitmapString("RED", 5+30, 5+30);
    
    ofSetColor(255);
    image.draw(320+10,5,320,240);
    ofSetColor(ofColor::green);
    ofDrawBitmapString("GREEN", 320+10+30,5+30);
    
    ofSetColor(255);
    movie.draw(320*2+15,5,320,240);
    ofSetColor(ofColor::blue);
    ofDrawBitmapString("BLUE", 320*2+5+30,5+30);
    
    ofSetColor(255);
    maskFbo.draw(320+10,240+10,320,240);
    ofDrawBitmapString("RGB MASK", 320+10+30,240+10+30);
    
    fbo.draw(320+10,240*2+15,320,240);
    ofDrawBitmapString("Final FBO", 320+10+30,240*2+15+30);
}

//--------------------------------------------------------------
void testApp::keyPressed(int key){
    
}

//--------------------------------------------------------------
void testApp::keyReleased(int key){

}

//--------------------------------------------------------------
void testApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void testApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void testApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void testApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void testApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void testApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void testApp::dragEvent(ofDragInfo dragInfo){ 

}
