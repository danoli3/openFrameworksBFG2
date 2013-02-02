#include "ofxLabel.h"
#include "ofGraphics.h"

ofxLabel::ofxLabel(ofParameter<string> _label, float width, float height){
	setup(_label,width,height);
}

ofxLabel* ofxLabel::setup(ofParameter<string> _label, float width, float height) {
    label.makeReferenceTo(_label);
    b.width  = width;
    b.height = height;
    return this;
}

ofxLabel* ofxLabel::setup(string labelName, string _label, float width, float height) {
    label.set(labelName,_label);
    return setup(label,width,height);
}

void ofxLabel::draw() {
    currentFrame = ofGetFrameNum();

	ofColor c = ofGetStyle().color;
    ofPushMatrix();

    ofSetColor(backgroundColor);
    ofRect(b);

    ofTranslate(b.x, b.y);
    ofSetColor(textColor);
    ofTranslate(0, b.height / 2 + 4);
    string name;
    if(!getName().empty()){
    	name = getName() + ": ";
    }
    font.drawString(name + (string)label, textPadding, 0);

    ofPopMatrix();
    ofSetColor(c);
}

ofAbstractParameter & ofxLabel::getParameter(){
	return label;
}
