#pragma once

#include "ofMesh.h"
#include "ofVbo.h"

class ofVboMesh: public ofMesh{
public:
	ofVboMesh();
	ofVboMesh(const ofMesh & mom);
	void setUsage(int usage);
	void draw(ofPolyRenderMode drawMode);

private:
	ofVbo vbo;
	int usage;
};
