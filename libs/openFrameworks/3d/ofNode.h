
#pragma once

#include "ofVectorMath.h"

// TODO: look at optimizing matrix updates

// Info:
// Nodes 'look' along -ve z axis


// a generic 3d object in space with transformation (position, rotation, scale)
// with API to move around in global or local space
// and virtual draw methods

class ofNode {
public:
	ofNode();
	virtual ~ofNode() {}

	// set parent to link nodes
	// transformations are inherited from parent node
	// set to NULL if not needed (default)
	void setParent(ofNode* parent);
	ofNode* getParent() const;

	
	//----------------------------------------
	// Get transformations
	
	ofVec3f getPosition() const;
	float getX() const;
	float getY() const;
	float getZ() const;
	
	ofVec3f getXAxis() const;
	ofVec3f getYAxis() const;
	ofVec3f getZAxis() const;
	
	ofVec3f getSideDir() const;		// x axis
	ofVec3f getLookAtDir()const;	// -z axis
	ofVec3f getUpDir() const;		// y axis
	
	float getPitch() const;
	float getHeading() const;
	float getRoll() const;
	
	ofQuaternion getOrientationQuat() const;
	ofVec3f getOrientationEuler() const;
	//	ofMatrix3x3 getOrientationMatrix() const;
	
	ofVec3f getScale() const;

	
	const ofMatrix4x4& getMatrix();
	
	// TODO: optimize and cache these
	ofMatrix4x4 getGlobalMatrix();
	ofVec3f getGlobalPosition();
	ofQuaternion getGlobalOrientation();
//	ofVec3f getGlobalScale();

	
	
	// Set Transformations

	// directly set transformation matrix
	// TODO:
	void setMatrix(ofMatrix4x4 &m44) {}
	void setMatrix(float *m44) {}
	
	// position
	void setPosition(float px, float py, float pz);
	void setPosition(const ofVec3f& p);

	// orientation
	void setOrientation(const ofQuaternion& q);			// set as quaternion
	void setOrientation(const ofVec3f& eulerAngles);	// or euler can be useful, but prepare for gimbal lock
//	void setOrientation(const ofMatrix3x3& orientation);// or set as m33 if you have transformation matrix
	
	// scale set and get
	void setScale(float s);
	void setScale(float sx, float sy, float sz);
	void setScale(const ofVec3f& s);
	
	
	// helpful move methods
	void move(float x, float y, float z);			// move by arbitrary amount
	void move(const ofVec3f& offset);				// move by arbitrary amount
	void truck(float amount);						// move sideways (in local x axis)
	void boom(float amount);						// move up+down (in local y axis)
	void dolly(float amount);						// move forward+backward (in local z axis)
	
	
	// helpful rotation methods
	void tilt(float degrees);						// tilt up+down (around local x axis)
	void pan(float degrees);						// rotate left+right (around local y axis)
	void roll(float degrees);						// roll left+right (around local z axis)
	void rotate(const ofQuaternion& q);				// rotate by quaternion
	void rotate(float degrees, const ofVec3f& v);	// rotate around arbitrary axis by angle
	void rotate(float degrees, float vx, float vy, float vz);
	
	void rotateAround(const ofQuaternion& q, const ofVec3f& point);	// rotate by quaternion around point
	void rotateAround(float degrees, const ofVec3f& axis, const ofVec3f& point);	// rotate around arbitrary axis by angle around point
	
	// orient node to look at position (-ve z axis pointing to node)
	void lookAt(const ofVec3f& lookAtPosition, const ofVec3f& upVector = ofVec3f(0, 1, 0));
	void lookAt(ofNode& lookAtNode, const ofVec3f& upVector = ofVec3f(0, 1, 0));
	
	
	// orbit object around target at radius
	void orbit(float longitude, float latitude, float radius, const ofVec3f& centerPoint = ofVec3f(0, 0, 0));
	void orbit(float longitude, float latitude, float radius, ofNode& centerNode);
	
	
	// set opengl's modelview matrix to this nodes transform
	// if you want to draw something at the position+orientation+scale of this node...
	// ...call ofNode::transform(); write your draw code, and ofNode::restoreTransform();
	// OR A simpler way is to extend ofNode and override ofNode::customDraw();
	void transform();
	void restoreTransform();
	
	
	// resets this node's transformation
	void resetTransform();
	

	// if you extend ofNode and wish to change the way it draws, extend these
	virtual void customDraw() {}
	virtual void customDebugDraw();
	
	
	// draw functions. do NOT override these
	// transforms the node to its position+orientation+scale
	// and calls the virtual 'customDraw' method which you CAN override
	void draw();
	void debugDraw();
	
protected:
	ofNode *parent;
	
	ofVec3f position;
	ofQuaternion orientation;
	ofVec3f scale;
	
	ofVec3f axis[3];
	
	virtual void updateMatrix();
	

private:
//	bool		isMatrixDirty;
	ofMatrix4x4 transformMatrix;
};
