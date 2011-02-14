#include "testApp.h"


//--------------------------------------------------------------
void testApp::setup(){
	dir.listDir("images/of_logos");
	
	// you can now iterate through the files as you like
	for(int i = 0; i < dir.size(); i++){
		// dynamically create an ofImage*
		ofImage* curImage = new ofImage();
		// load an image into the ofImage*
		curImage->loadImage(dir.getPath(i));
		// add the image to the array of images
		images.push_back(curImage);
	}
	currentImage = 0;
	
	ofBackground(ofColor::white);
	
}

//--------------------------------------------------------------
void testApp::update(){
	
}

//--------------------------------------------------------------
void testApp::draw(){
	
	if (dir.size() > 0){
		ofSetColor(ofColor::white);
		images[currentImage]->draw(300,50);
		
		ofSetColor(ofColor::gray);
		string pathInfo = dir.getPath(currentImage) + "\n\n" +
			"press any key to advance current image\n\n" +
			"many thanks to hikaru furuhashi for the OFs";
		ofDrawBitmapString(pathInfo, 300, images[currentImage]->getHeight() + 80);
	}
	
	ofSetColor(ofColor::gray);
	for(int i = 0; i < dir.size(); i++){
		if(i == currentImage) {
			ofSetColor(ofColor::red);
		}	else {
			ofSetColor(ofColor::black);
		}
		string fileInfo = "file " + ofToString(i + 1) + " = " + dir.getName(i);
		ofDrawBitmapString(fileInfo, 50,i * 20 + 50);
	}
	
}

//--------------------------------------------------------------
void testApp::keyPressed(int key){
	if (dir.size() > 0){
		currentImage++;
		currentImage %= dir.size();
	}
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
