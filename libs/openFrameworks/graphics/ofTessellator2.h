#pragma once

#include "ofConstants.h"
#include "ofMesh.h"
#include "ofTypes.h"
#include "ofPath.h"
#include "tesselator.h"


/** ofTessellator
 
 Tessellates a polyline into a number of meshes, conforming to the given winding rule.
 
 @author Adapted by damian from Theo's ofxShape.
 */



class ofTessellator2
{
public:	
	
	ofTessellator2();
	~ofTessellator2();
	ofTessellator2(const ofTessellator2 & mom);
	ofTessellator2 & operator=(const ofTessellator2 & mom);

	/// tessellate polyline and return a mesh. if bIs2D==true, do a 10% more efficient normal calculation.
	void tessellateToMesh( const vector<ofPolyline>& src, ofPolyWindingMode polyWindingMode, ofMesh & dstmesh, bool bIs2D=false );
	void tessellateToMesh( const ofPolyline& src,  ofPolyWindingMode polyWindingMode, ofMesh& dstmesh, bool bIs2D=false );

private:
	
	void performTessellation( const vector<ofPolyline>& polylines, ofPolyWindingMode polyWindingMode, ofMesh& dstmesh, bool bIs2D );
	void init();

	TESStesselator * cacheTess;
	TESSalloc tessAllocator;
};


