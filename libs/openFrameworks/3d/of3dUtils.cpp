#include "of3dUtils.h"
#include "ofGraphics.h"

void ofDrawAxis(float size) {
	ofPushStyle();
	ofSetLineWidth(3);

	// draw x axis
	ofSetColor(ofColor::red);
	ofLine(0, 0, 0, size, 0, 0);
	
	// draw y axis
	ofSetColor(ofColor::green);
	ofLine(0, 0, 0, 0, size, 0);

	// draw z axis
	ofSetColor(ofColor::blue);
	ofLine(0, 0, 0, 0, 0, size);
	
	ofPopStyle();
}

//--------------------------------------------------------------
void ofDrawGrid(float scale, float ticks, bool labels, bool x, bool y, bool z) {
	
	ofColor c = ofGetStyle().color;
	if (c == ofColor::white)
		c = ofColor(255,0,0);
	
	ofPushStyle();
	
	if (x) {
		c.setHue(0.0f);
		c.setSaturation(190);		
		ofSetColor(c);
		ofDrawGridPlane(scale, ticks, labels);
	}
	if (y) {
		c.setHue(255.0f / 3.0f);
		c.setSaturation(190);		
		ofSetColor(c);
		ofPushMatrix();
		ofRotate(90, 0, 0, -1);
		ofDrawGridPlane(scale, ticks, labels);
		ofPopMatrix();
	}
	if (z) {
		c.setHue(255.0f * 2.0f / 3.0f);
		c.setSaturation(190);		
		ofSetColor(c);
		ofPushMatrix();
		ofRotate(90, 0, 1, 0);
		ofDrawGridPlane(scale, ticks, labels);
		ofPopMatrix();
	}
	
	if (labels) {
		ofPushStyle();
		ofSetColor(255, 255, 255);
		float labelPos = scale * (1.0f + 0.5f / ticks);
		ofSetDrawBitmapMode(OF_BITMAPMODE_MODEL_BILLBOARD);
		ofDrawBitmapString("x", labelPos, 0, 0);
		ofDrawBitmapString("y", 0, labelPos, 0);
		ofDrawBitmapString("z", 0, 0, labelPos);
		ofPopStyle();
	}
	ofPopStyle();
}


//--------------------------------------------------------------
void ofDrawGridPlane(float scale, float ticks, bool labels) {
	
	float minor = scale / ticks;
	float major =  minor * 2.0f;
	
	ofPushStyle();
	for (int iDimension=0; iDimension<2; iDimension++)
	{
		for (float yz=-scale; yz<=scale; yz+= minor)
		{
			//major major
			if (fabs(yz) == scale || yz == 0)
				ofSetLineWidth(2.5);
			
			//major
			else if (yz / major == floor(yz / major) )
				ofSetLineWidth(1.5);
			
			//minor
			else
				ofSetLineWidth(1);
			if (iDimension==0)
				ofLine(0, yz, -scale, 0, yz, scale);
			else
				ofLine(0, -scale, yz, 0, scale, yz);
		}
	}
	ofPopStyle();
	
	if (labels) {
		//draw numbers on axes
		ofPushStyle();
		ofSetColor(255, 255, 255);
		
		float accuracy = ceil(-log(scale/ticks)/log(10.0f));
		
		ofSetDrawBitmapMode(OF_BITMAPMODE_MODEL_BILLBOARD);
		for (float yz = -scale; yz<=scale; yz+=minor)
		{
			ofDrawBitmapString(ofToString(yz, accuracy), 0, yz, 0);
			ofDrawBitmapString(ofToString(yz, accuracy), 0, 0, yz);		
		}
		ofPopStyle();
	}
	
}

//--------------------------------------------------------------
void ofDrawArrow(const ofVec3f& start, const ofVec3f& end, float headSize) {
	
	//draw line
	ofLine(start, end);
	
	//draw cone
	ofMatrix4x4 mat;
	mat.makeRotationMatrix(ofVec3f(0,0,1), end - start);
	ofPushMatrix();
	ofTranslate(end);
	glMultMatrixf(mat.getPtr());
	ofTranslate(0,0,-headSize);
	ofCone(headSize, headSize);	
	ofPopMatrix();
}