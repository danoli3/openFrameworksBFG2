#include "ofProgrammableGLRenderer.h"
#include "ofMesh.h"
#include "ofPath.h"
#include "ofGraphics.h"
#include "ofAppRunner.h"
#include "ofMesh.h"
#include "ofBitmapFont.h"
#include "ofGLUtils.h"
#include "ofImage.h"
#include "ofFbo.h"

#ifdef TARGET_OPENGLES
string defaultVertexShader =
		"attribute vec4 position;\
		attribute vec4 color;\
		attribute vec4 normal;\
		attribute vec2 texcoord;\
		\
		uniform mat4 modelViewMatrix;\
		uniform mat4 projectionMatrix;\
		\
		varying vec4 colorVarying;\
		varying vec2 texCoordVarying;\
        \
		void main(){\
			gl_Position = projectionMatrix * modelViewMatrix * position;\
			colorVarying = color;\
			texCoordVarying = texcoord;\
		}";


string defaultFragmentShader =
		"#ifdef GL_ES\n\
		// define default precision for float, vec, mat.\n\
		precision highp float;\n\
		#endif\n\
		\
		uniform sampler2D src_tex_unit0;\
		uniform float useTexture;\
		uniform float useColors;\
		uniform vec4 color;\
		\
		varying float depth;\
		varying vec4 colorVarying;\
		varying vec2 texCoordVarying;\
		\
		void main(){\
			vec4 c;\
			if(useColors>0.5){\
				c = colorVarying;\
			}else{\n\
				c = color;\
			}\n\
			if(useTexture>0.5){\
				gl_FragColor = texture2D(src_tex_unit0, texCoordVarying)*c;\
			}else{\n\
				gl_FragColor = c;\
			}\
		}";
#else
string defaultVertexShader =
"#version 150\n\
\n\
uniform mat4 projectionMatrix;\n\
uniform mat4 modelViewMatrix;\n\
\n\
uniform vec4 uColor;\n\
\n\
\n\
in vec4  position;\n\
in vec4  normal;\n\
in vec2  texCoord;\n\
in vec4  color;\n\
\n\
out vec4 colorVarying;\n\
out vec2 texCoordVarying;\n\
\n\
void main()\n\
{\n\
	colorVarying = uColor;\n\
	\n\
	colorVarying = vec4((normal.xyz + vec3(1.0, 1.0, 1.0)) / 2.0,1.0);\n\
	//	vertColor = vec4(1.0);\n\
	colorVarying.a = 1.0;\n\
	texCoordVarying = texCoord;\n\
	gl_Position = projectionMatrix * modelViewMatrix * position;\n\
}";


string defaultFragmentShader =
"#version 150\n\
\n\
uniform sampler2D src_tex_unit0;\n\
uniform float useTexture;\n\
uniform float useColors;\n\
uniform vec4 color;\n\
\n\
in float depth;\n\
in vec4 colorVarying;\n\
in vec2 texCoordVarying;\n\
out vec4 fragColor;\n\
\n\
void main(){\n\
vec4 c = vec4(1.0);\n\
if(useColors>0.5){\n\
c = colorVarying;\n\
}else{\n\
c = color;\n\
}\n\
if(useTexture>0.5){\n\
fragColor = mix(texture(src_tex_unit0, texCoordVarying),c,0.5);\n\
}else{\n\
fragColor = c;\n\
}\n\
}";
#endif


//----------------------------------------------------------
ofProgrammableGLRenderer::ofProgrammableGLRenderer(string vertexShader, string fragmentShader, bool useShapeColor){
	bBackgroundAuto = true;

	linePoints.resize(2);
	triPoints.resize(3);
	rectPoints.resize(4);

	currentFbo = NULL;

    rectMode = OF_RECTMODE_CORNER;
    bFilled = OF_FILLED;
    coordHandedness = OF_LEFT_HANDED;
    bSmoothHinted = false;
    currentMatrixMode = OF_MATRIX_MODELVIEW;

    vertexFile = vertexShader;
    fragmentFile = fragmentShader;

    verticesEnabled = true;
    colorsEnabled = false;
    texCoordsEnabled = false;
    normalsEnabled = false;

#ifndef TARGET_OPENGLES
	glGenVertexArrays(1, &defaultVAO);
#else
	glGenVertexArraysOES(1, &defaultVAO);
#endif


}

ofProgrammableGLRenderer::~ofProgrammableGLRenderer() {

#ifndef TARGET_OPENGLES
	glDeleteVertexArrays(1, &defaultVAO);
#else
	glDeleteVertexArraysOES(1, &defaultVAO);
#endif

}

//----------------------------------------------------------
void ofProgrammableGLRenderer::startRender() {

	// bind vertex array
#ifndef TARGET_OPENGLES
	glBindVertexArray(defaultVAO);
#else
	glBindVertexArrayOES(defaultVAO);
#endif

	currentShader.begin();
}

//----------------------------------------------------------
void ofProgrammableGLRenderer::finishRender() {
	currentShader.end();
	modelViewStack.empty();
	projectionStack.empty();
}

//----------------------------------------------------------
bool ofProgrammableGLRenderer::setup() {
	bool ret;
	if(vertexFile!=""){
		ofLogNotice() << "GLES2 loading vertex shader from " + vertexFile;
		ret = defaultShader.setupShaderFromFile(GL_VERTEX_SHADER,vertexFile);
	}else{
		ofLogNotice() << "GLES2 loading vertex shader from default source";
		ret = defaultShader.setupShaderFromSource(GL_VERTEX_SHADER,defaultVertexShader);
	}
	if(ret){
		if(fragmentFile!=""){
			ofLogNotice() << "GLES2 loading fragment shader from " + fragmentFile;
			ret = defaultShader.setupShaderFromFile(GL_FRAGMENT_SHADER,fragmentFile);
		}else{
			ofLogNotice() << "GLES2 loading fragment shader from default source";
			ret = defaultShader.setupShaderFromSource(GL_FRAGMENT_SHADER,defaultFragmentShader);
		}
	}
	if(ret){
		ret = defaultShader.linkProgram();
	}
	currentShader = defaultShader;
	return ret;
}

//----------------------------------------------------------
void ofProgrammableGLRenderer::update(){
    //
}

//----------------------------------------------------------
void ofProgrammableGLRenderer::draw(ofMesh & vertexData, bool useColors, bool useTextures, bool useNormals){
	draw(vertexData, OF_MESH_FILL, useColors, useTextures, useNormals); // tig: use default mode if no render mode specified.
}

//----------------------------------------------------------
void ofProgrammableGLRenderer::draw(ofMesh & vertexData, ofPolyRenderMode renderType, bool useColors, bool useTextures, bool useNormals){

	// tig: note that there is a lot of code duplication going on here.
	// the most elegant way to do it would be to do a good old
	// 	meshVbo.setMesh(vertexData, GL_DYNAMIC_DRAW);
	// and then render the vbo by calling its .draw() method.
	// we can't do that, however, since vbo::draw() doesn't allow us to specify
	// whether we should use Colors, Textures, Normals.

	// I have another idea.
	
	// call .disableColours() if needed...
	
	
	if(!vertexData.hasVertices()){
		ofLogVerbose() << "Cannot draw a mesh without vertices.";
		return;
	}

	if (bSmoothHinted) startSmoothing();
	meshVbo.clear();
	
	meshVbo.setVertexData( vertexData.getVerticesPointer(), vertexData.getNumVertices(), GL_DYNAMIC_DRAW);
	preparePrimitiveDraw(meshVbo);
	
	if(useNormals && vertexData.hasNormals()){
		meshVbo.setNormalData(vertexData.getNormalsPointer(), vertexData.getNumNormals(), GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, meshVbo.getNormalId()); // bind to normals from vbo
		enableNormals();
		glVertexAttribPointer(getAttrLocationNormal(), 3, GL_FLOAT,GL_FALSE,0,0);	// upload normals
	} else {
		disableNormals();
		glBindBuffer(GL_ARRAY_BUFFER,0);
	}
	
	if(useColors && vertexData.hasColors()){
		meshVbo.setColorData(vertexData.getColorsPointer(),vertexData.getNumColors(),GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, meshVbo.getColorId()); // bind colors from vbo
		enableColors();
		glVertexAttribPointer(getAttrLocationColor(), 4 , GL_FLOAT, GL_FALSE,0,0);
	} else {
		disableColors();
		glBindBuffer(GL_ARRAY_BUFFER,0);
	}
	
	if(useTextures && vertexData.hasTexCoords()){
		meshVbo.setTexCoordData(vertexData.getTexCoordsPointer(), vertexData.getNumTexCoords(), GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, meshVbo.getColorId()); // bind to triangle vertices
		enableTexCoords();
		glVertexAttribPointer(getAttrLocationTexCoord(), 2, GL_FLOAT,GL_FALSE,0,0);

	} else {
		disableTexCoords();
		glBindBuffer(GL_ARRAY_BUFFER,0);
	}
	
	if (vertexData.hasIndices()){
		meshVbo.setIndexData(vertexData.getIndexPointer(), vertexData.getNumIndices(), GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshVbo.getIndexId());
	}
	
	GLenum drawMode;
	
#ifdef TARGET_OPENGLES
	switch(renderType){
		case OF_MESH_POINTS:
			drawMode = GL_POINTS;
			break;
		case OF_MESH_WIREFRAME:
			drawMode = GL_LINES;
			break;
		case OF_MESH_FILL:
			drawMode = vertexData.getMode();
			break;
		default:
			// use the current fill mode to tell.
			drawMode = vertexData.getMode();
			break;
	}
	if(vertexData.hasIndices()){
		glDrawElements(drawMode, vertexData.getNumIndices(),GL_UNSIGNED_SHORT,NULL);
	} else {
		glDrawArrays(drawMode, 0, vertexData.getNumVertices());
	}
#else
	
	// OpenGL
	
	// tig: note that for GL3+ we use glPolygonMode to draw wireframes or filled meshes, and not the primitive mode.
	// the reason is not purely aesthetic, but more conformant with the behaviour of ofGLRenderer. Whereas
	// gles2.0 doesn't allow for a polygonmode.
	glPolygonMode(GL_FRONT_AND_BACK, ofGetGLPolyMode(renderType));
	if(vertexData.hasIndices()) {
		glDrawElements(ofGetGLPrimitiveMode(vertexData.getMode()), vertexData.getNumIndices(),GL_UNSIGNED_INT,NULL);
	}
	else {
		glDrawArrays(ofGetGLPrimitiveMode(vertexData.getMode()), 0, vertexData.getNumVertices());
	}
		// tig: note further that we could query and store the current polygon mode, but don't, since that would
	// infer a massive performance penalty. instead, we revert the glPolygonMode to mirror the current ofFill state
	// after we're finished drawing, following the principle of least surprise.
	glPolygonMode(GL_FRONT_AND_BACK, (ofGetFill() == OF_OUTLINE) ?  GL_LINE : GL_FILL);
	
#endif
	
	if(vertexData.getNumColors()){
		disableColors();
	}
	if(vertexData.getNumNormals()){
		disableNormals();
	}
	if(vertexData.getNumTexCoords()){
		disableTexCoords();
	}
	if (bSmoothHinted) endSmoothing();
	
	finishPrimitiveDraw();
}

//----------------------------------------------------------
void ofProgrammableGLRenderer::draw(vector<ofPoint> & vertexData, ofPrimitiveMode drawMode){
	if(!vertexData.empty()) {
		if (bSmoothHinted) startSmoothing();
		disableTexCoords();
		disableColors();
		vertexDataVbo.setVertexData(&vertexData[0], vertexData.size(), GL_DYNAMIC_DRAW);
		preparePrimitiveDraw(vertexDataVbo);
		glDrawArrays(ofGetGLPrimitiveMode(drawMode), 0, vertexData.size());
		finishPrimitiveDraw();
		if (bSmoothHinted) endSmoothing();
	}
}

//----------------------------------------------------------
void ofProgrammableGLRenderer::draw(ofPolyline & poly){
	if(!poly.getVertices().empty()) {
		// use smoothness, if requested:
		if (bSmoothHinted) startSmoothing();

		disableTexCoords();
		disableColors();

		vertexDataVbo.setVertexData(&poly.getVertices()[0], poly.size(), GL_DYNAMIC_DRAW);
		preparePrimitiveDraw(vertexDataVbo);
		glDrawArrays(poly.isClosed()?GL_LINE_LOOP:GL_LINE_STRIP, 0, poly.size());
		finishPrimitiveDraw();

		// use smoothness, if requested:
		if (bSmoothHinted) endSmoothing();
	}
}

//----------------------------------------------------------
void ofProgrammableGLRenderer::draw(ofPath & shape){
	ofColor prevColor;
	if(shape.getUseShapeColor()){
		prevColor = ofGetStyle().color;
	}
	if(shape.isFilled()){
		ofMesh & mesh = shape.getTessellation();
		if(shape.getUseShapeColor()){
			setColor( shape.getFillColor() * ofGetStyle().color,shape.getFillColor().a/255. * ofGetStyle().color.a);
		}
		draw(mesh);
	}
	if(shape.hasOutline()){
		float lineWidth = ofGetStyle().lineWidth;
		if(shape.getUseShapeColor()){
			setColor( shape.getStrokeColor() * ofGetStyle().color, shape.getStrokeColor().a/255. * ofGetStyle().color.a);
		}
		setLineWidth( shape.getStrokeWidth() );
		vector<ofPolyline> & outlines = shape.getOutline();
		for(int i=0; i<(int)outlines.size(); i++)
			draw(outlines[i]);
		setLineWidth(lineWidth);
	}
	if(shape.getUseShapeColor()){
		setColor(prevColor);
	}
}

//----------------------------------------------------------
void ofProgrammableGLRenderer::draw(ofImage & image, float x, float y, float z, float w, float h, float sx, float sy, float sw, float sh){
	if(image.isUsingTexture()){
		enableTexCoords();
		disableColors();
		ofTexture& tex = image.getTextureReference();
		if(tex.bAllocated()) {
			tex.drawSubsection(x,y,z,w,h,sx,sy,sw,sh);
		} else {
			ofLogWarning() << "ofGLRenderer::draw(): texture is not allocated";
		}
		disableTexCoords();
	}
}

//----------------------------------------------------------
void ofProgrammableGLRenderer::draw(ofFloatImage & image, float x, float y, float z, float w, float h, float sx, float sy, float sw, float sh){
	if(image.isUsingTexture()){
		enableTexCoords();
		disableColors();
		ofTexture& tex = image.getTextureReference();
		if(tex.bAllocated()) {
			tex.drawSubsection(x,y,z,w,h,sx,sy,sw,sh);
		} else {
			ofLogWarning() << "ofGLRenderer::draw(): texture is not allocated";
		}
		disableTexCoords();
	}
}

//----------------------------------------------------------
void ofProgrammableGLRenderer::draw(ofShortImage & image, float x, float y, float z, float w, float h, float sx, float sy, float sw, float sh){
	if(image.isUsingTexture()){
		enableTexCoords();
		disableColors();
		ofTexture& tex = image.getTextureReference();
		if(tex.bAllocated()) {
			tex.drawSubsection(x,y,z,w,h,sx,sy,sw,sh);
		} else {
			ofLogWarning() << "ofGLRenderer::draw(): texture is not allocated";
		}
		disableTexCoords();
	}
}

//----------------------------------------------------------
void ofProgrammableGLRenderer::setCurrentFBO(ofFbo * fbo){
	currentFbo = fbo;
}

//----------------------------------------------------------
void ofProgrammableGLRenderer::pushView() {
	viewportHistory.push(currentViewport);
	ofMatrixMode currentMode = currentMatrixMode;
	matrixMode(OF_MATRIX_PROJECTION);
	pushMatrix();
	matrixMode(OF_MATRIX_MODELVIEW);
	pushMatrix();
	matrixMode(currentMode);
}

//----------------------------------------------------------
void ofProgrammableGLRenderer::popView() {
	if( viewportHistory.size() ){
		ofRectangle viewRect = viewportHistory.top();
		viewport(viewRect.x, viewRect.y, viewRect.width, viewRect.height,false);
		viewportHistory.pop();
	}
	ofMatrixMode currentMode = currentMatrixMode;
	matrixMode(OF_MATRIX_PROJECTION);
	popMatrix();
	matrixMode(OF_MATRIX_MODELVIEW);
	popMatrix();
	matrixMode(currentMode);
}

//----------------------------------------------------------
void ofProgrammableGLRenderer::viewport(ofRectangle viewport_){
	viewport(viewport_.x, viewport_.y, viewport_.width, viewport_.height,true);
}

//----------------------------------------------------------
void ofProgrammableGLRenderer::viewport(float x, float y, float width, float height, bool invertY) {
	if(width == 0) width = ofGetWindowWidth();
	if(height == 0) height = ofGetWindowHeight();

	if (invertY){
		if(currentFbo){
			y = currentFbo->getHeight() - (y + height);
		}else{
			y = ofGetWindowHeight() - (y + height);
		}
	}
	currentViewport.set(x, y, width, height);
	glViewport(x, y, width, height);
}

//----------------------------------------------------------
ofRectangle ofProgrammableGLRenderer::getCurrentViewport(){
    return currentViewport;
}

//----------------------------------------------------------
int ofProgrammableGLRenderer::getViewportWidth(){
	return currentViewport.width;
}

//----------------------------------------------------------
int ofProgrammableGLRenderer::getViewportHeight(){
	return currentViewport.height;
}

//----------------------------------------------------------
void ofProgrammableGLRenderer::setCoordHandedness(ofHandednessType handedness) {
	coordHandedness = handedness;
}

//----------------------------------------------------------
ofHandednessType ofProgrammableGLRenderer::getCoordHandedness() {
	return coordHandedness;
}

//----------------------------------------------------------
void ofProgrammableGLRenderer::setOrientationMatrix(float width, float height, ofOrientation orientation, bool vFlip){
	orientationMatrix.makeIdentityMatrix();

	//note - theo checked this on iPhone and Desktop for both vFlip = false and true
	if(ofDoesHWOrientation()){
		if(vFlip){
			orientationMatrix.glScale(1, -1, 1);
			orientationMatrix.glTranslate(0, -height, 0);
		}
	}else{
		if( orientation == OF_ORIENTATION_UNKNOWN ) orientation = ofGetOrientation();
		switch(orientation) {
			case OF_ORIENTATION_180:
				orientationMatrix.glRotate(-180, 0, 0, 1);
				if(vFlip){
					orientationMatrix.glScale(1, -1, 1);
					orientationMatrix.glTranslate(-width, 0, 0);
				}else{
					orientationMatrix.glTranslate(-width, -height, 0);
				}

				break;

			case OF_ORIENTATION_90_RIGHT:
				orientationMatrix.glRotate(-90, 0, 0, 1);
				if(vFlip){
					orientationMatrix.glScale(-1, 1, 1);
				}else{
					orientationMatrix.glScale(-1, -1, 1);
					orientationMatrix.glTranslate(0, -height, 0);
				}
				break;

			case OF_ORIENTATION_90_LEFT:
				orientationMatrix.glRotate(90, 0, 0, 1);
				if(vFlip){
					orientationMatrix.glScale(-1, 1, 1);
					orientationMatrix.glTranslate(-width, -height, 0);
				}else{
					orientationMatrix.glScale(-1, -1, 1);
					orientationMatrix.glTranslate(-width, 0, 0);
				}
				break;

			case OF_ORIENTATION_DEFAULT:
			default:
				if(vFlip){
					orientationMatrix.glScale(1, -1, 1);
					orientationMatrix.glTranslate(0, -height, 0);
				}
				break;
		}
	}
}

//----------------------------------------------------------
void ofProgrammableGLRenderer::setupScreenPerspective(float width, float height, ofOrientation orientation, bool vFlip, float fov, float nearDist, float farDist) {

	if(width == 0) width = ofGetWidth();
	if(height == 0) height = ofGetHeight();

	float viewW = getViewportWidth();
	float viewH = getViewportHeight();

	float eyeX = viewW / 2;
	float eyeY = viewH / 2;
	float halfFov = PI * fov / 360;
	float theTan = tanf(halfFov);
	float dist = eyeY / theTan;
	float aspect = (float) viewW / viewH;

	if(nearDist == 0) nearDist = dist / 10.0f;
	if(farDist == 0) farDist = dist * 10.0f;

	projection.makePerspectiveMatrix(fov, aspect, nearDist, farDist);
	modelView.makeLookAtViewMatrix(ofVec3f(eyeX, eyeY, dist),  ofVec3f(eyeX, eyeY, 0),  ofVec3f(0, 1, 0));
	setOrientationMatrix(width,height,orientation,vFlip);

	modelViewOrientation = modelView;
	modelViewOrientation.preMult(orientationMatrix);
	uploadModelViewMatrix(modelViewOrientation);
	uploadProjectionMatrix(projection);
	matrixMode(OF_MATRIX_MODELVIEW);
}

//----------------------------------------------------------
void ofProgrammableGLRenderer::setupScreenOrtho(float width, float height, ofOrientation orientation, bool vFlip, float nearDist, float farDist) {
	if(width == 0) width = ofGetWidth();
	if(height == 0) height = ofGetHeight();

	float viewW = ofGetViewportWidth();
	float viewH = ofGetViewportHeight();


	ofSetCoordHandedness(OF_RIGHT_HANDED);
	if(vFlip) {
		projection = ofMatrix4x4::newOrthoMatrix(0, width, height, 0, nearDist, farDist);
		ofSetCoordHandedness(OF_LEFT_HANDED);
	}else{
		projection = ofMatrix4x4::newOrthoMatrix(0, viewW, 0, viewH, nearDist, farDist);
	}

	setOrientationMatrix(width,height,orientation,vFlip);

	uploadProjectionMatrix(projection);

	matrixMode(OF_MATRIX_MODELVIEW);
	loadIdentityMatrix();
}

//----------------------------------------------------------
//Resets openGL parameters back to OF defaults
void ofProgrammableGLRenderer::setupGraphicDefaults(){
	/*glEnableVertexAttribArray(getAttrLocationPosition());
	glDisableVertexAttribArray(getAttrLocationColor());
	glDisableVertexAttribArray(getAttrLocationNormal());
	glDisableVertexAttribArray(getAttrLocationTexCoord());*/
}

//----------------------------------------------------------
void ofProgrammableGLRenderer::setupScreen(){
	setupScreenPerspective();	// assume defaults
}

//----------------------------------------------------------
void ofProgrammableGLRenderer::setCircleResolution(int res){
	if((int)circlePolyline.size()!=res+1){
		circlePolyline.clear();
		circlePolyline.arc(0,0,0,1,1,0,360,res);
		circlePoints.resize(circlePolyline.size());
	}
}

//----------------------------------------------------------
void ofProgrammableGLRenderer::setSphereResolution(int res) {
	if(sphereMesh.getNumVertices() == 0 || res != ofGetStyle().sphereResolution) {
		int n = res * 2;
		float ndiv2=(float)n/2;
        
		/*
		 Original code by Paul Bourke
		 A more efficient contribution by Federico Dosil (below)
		 Draw a point for zero radius spheres
		 Use CCW facet ordering
		 http://paulbourke.net/texture_colour/texturemap/
		 */
		
		float theta2 = TWO_PI;
		float phi1 = -HALF_PI;
		float phi2 = HALF_PI;
		float r = 1.f; // normalize the verts //
        
		sphereMesh.clear();
        sphereMesh.setMode(OF_PRIMITIVE_TRIANGLE_STRIP);
        
		int i, j;
        float theta1 = 0.f;
		float jdivn,j1divn,idivn,dosdivn,unodivn=1/(float)n,t1,t2,t3,cost1,cost2,cte1,cte3;
		cte3 = (theta2-theta1)/n;
		cte1 = (phi2-phi1)/ndiv2;
		dosdivn = 2*unodivn;
		ofVec3f e,p,e2,p2;
        
		if (n < 0){
			n = -n;
			ndiv2 = -ndiv2;
		}
		if (n < 4) {n = 4; ndiv2=(float)n/2;}
        if(r <= 0) r = 1;
		
		t2=phi1;
		cost2=cos(phi1);
		j1divn=0;
        
        ofVec3f vert, normal;
        ofVec2f tcoord;
		
		for (j=0;j<ndiv2;j++) {
			t1 = t2;
			t2 += cte1;
			t3 = theta1 - cte3;
			cost1 = cost2;
			cost2 = cos(t2);
			e.y = sin(t1);
			e2.y = sin(t2);
			p.y = r * e.y;
			p2.y = r * e2.y;
			
			idivn=0;
			jdivn=j1divn;
			j1divn+=dosdivn;
			for (i=0;i<=n;i++) {
				t3 += cte3;
				e.x = cost1 * cos(t3);
				e.z = cost1 * sin(t3);
				p.x = r * e.x;
				p.z = r * e.z;
				
				normal.set( e.x, e.y, e.z );
				tcoord.set( idivn, jdivn );
				vert.set( p.x, p.y, p.z );
				
				sphereMesh.addNormal(normal);
				sphereMesh.addTexCoord(tcoord);
				sphereMesh.addVertex(vert);
				
				e2.x = cost2 * cos(t3);
				e2.z = cost2 * sin(t3);
				p2.x = r * e2.x;
				p2.z = r * e2.z;
				
				normal.set(e2.x, e2.y, e2.z);
				tcoord.set(idivn, j1divn);
				vert.set(p2.x, p2.y, p2.z);
				
				sphereMesh.addNormal(normal);
				sphereMesh.addTexCoord(tcoord);
				sphereMesh.addVertex(vert);
				
				idivn += unodivn;
				
			}
		}
	}
}


//our openGL wrappers
//----------------------------------------------------------
void ofProgrammableGLRenderer::pushMatrix(){
	switch(currentMatrixMode){
	case OF_MATRIX_MODELVIEW:
		modelViewStack.push(modelViewOrientation);
		break;
	case OF_MATRIX_PROJECTION:
		projectionStack.push(projection);
		break;
	case OF_MATRIX_TEXTURE:
		textureStack.push(textureMatrix);
		break;
	}
}

//----------------------------------------------------------
void ofProgrammableGLRenderer::popMatrix(){
	switch(currentMatrixMode){
	case OF_MATRIX_MODELVIEW:
		modelViewOrientation = modelViewStack.top();
		uploadModelViewMatrix(modelViewOrientation);
		modelViewStack.pop();
		break;
	case OF_MATRIX_PROJECTION:
		projection = projectionStack.top();
		uploadProjectionMatrix(projection);
		projectionStack.pop();
		break;
	case OF_MATRIX_TEXTURE:
		textureMatrix = textureStack.top();
		uploadTextureMatrix(textureMatrix);
		textureStack.pop();
		break;
	}
}

//----------------------------------------------------------
void ofProgrammableGLRenderer::translate(const ofPoint& p){
	translate(p.x, p.y, p.z);
}

//----------------------------------------------------------
void ofProgrammableGLRenderer::translate(float x, float y, float z){
	switch(currentMatrixMode){
	case OF_MATRIX_MODELVIEW:
		modelViewOrientation.glTranslate(x,y,z);
		uploadModelViewMatrix(modelViewOrientation);
		break;
	case OF_MATRIX_PROJECTION:
		projection.glTranslate(x,y,z);
		uploadProjectionMatrix(projection);
		break;
	case OF_MATRIX_TEXTURE:
		textureMatrix.glTranslate(x,y,z);
		uploadTextureMatrix(textureMatrix);
		break;
	}
}

//----------------------------------------------------------
void ofProgrammableGLRenderer::scale(float xAmnt, float yAmnt, float zAmnt){
	switch(currentMatrixMode){
	case OF_MATRIX_MODELVIEW:
		modelViewOrientation.glScale(xAmnt,yAmnt,zAmnt);
		uploadModelViewMatrix(modelViewOrientation);
		break;
	case OF_MATRIX_PROJECTION:
		projection.glScale(xAmnt,yAmnt,zAmnt);
		uploadProjectionMatrix(projection);
		break;
	case OF_MATRIX_TEXTURE:
		textureMatrix.glScale(xAmnt,yAmnt,zAmnt);
		uploadTextureMatrix(textureMatrix);
		break;
	}
}

//----------------------------------------------------------
void ofProgrammableGLRenderer::rotate(float degrees, float vecX, float vecY, float vecZ){
	switch(currentMatrixMode){
	case OF_MATRIX_MODELVIEW:
		modelViewOrientation.glRotate(degrees,vecX,vecY,vecZ);
		uploadModelViewMatrix(modelViewOrientation);
		break;
	case OF_MATRIX_PROJECTION:
		projection.glRotate(degrees,vecX,vecY,vecZ);
		uploadProjectionMatrix(projection);
		break;
	case OF_MATRIX_TEXTURE:
		textureMatrix.glRotate(degrees,vecX,vecY,vecZ);
		uploadTextureMatrix(textureMatrix);
		break;
	}
}

//----------------------------------------------------------
void ofProgrammableGLRenderer::rotateX(float degrees){
	rotate(degrees, 1, 0, 0);
}

//----------------------------------------------------------
void ofProgrammableGLRenderer::rotateY(float degrees){
	rotate(degrees, 0, 1, 0);
}

//----------------------------------------------------------
void ofProgrammableGLRenderer::rotateZ(float degrees){
	rotate(degrees, 0, 0, 1);
}

//same as ofRotateZ
//----------------------------------------------------------
void ofProgrammableGLRenderer::rotate(float degrees){
	rotateZ(degrees);
}

//----------------------------------------------------------
void ofProgrammableGLRenderer::matrixMode(ofMatrixMode mode){
	currentMatrixMode = mode;
}

//----------------------------------------------------------
void ofProgrammableGLRenderer::loadIdentityMatrix (void){
	ofMatrix4x4 m;
	m.makeIdentityMatrix();
	loadMatrix(m);
}

//----------------------------------------------------------
void ofProgrammableGLRenderer::loadMatrix (const ofMatrix4x4 & m){
	switch(currentMatrixMode){
	case OF_MATRIX_MODELVIEW:
		modelView = m;
		modelViewOrientation = modelView;
		modelViewOrientation.preMult(orientationMatrix);
		uploadModelViewMatrix(modelViewOrientation);
		break;
	case OF_MATRIX_PROJECTION:
		projection = m;
		uploadProjectionMatrix(projection);
		break;
	case OF_MATRIX_TEXTURE:
		textureMatrix = m;
		uploadTextureMatrix(textureMatrix);
		break;
	}
}

//----------------------------------------------------------
void ofProgrammableGLRenderer::loadMatrix (const float *m){
	switch(currentMatrixMode){
	case OF_MATRIX_MODELVIEW:
		modelView.set(m);
		modelViewOrientation = modelView;
		modelViewOrientation.preMult(orientationMatrix);
		uploadModelViewMatrix(modelViewOrientation);
		break;
	case OF_MATRIX_PROJECTION:
		projection.set(m);
		uploadProjectionMatrix(projection);
		break;
	case OF_MATRIX_TEXTURE:
		textureMatrix.set(m);
		uploadTextureMatrix(textureMatrix);
		break;
	}
}

//----------------------------------------------------------
void ofProgrammableGLRenderer::multMatrix (const ofMatrix4x4 & m){
	switch(currentMatrixMode){
	case OF_MATRIX_MODELVIEW:
		modelViewOrientation.preMult(m);
		uploadModelViewMatrix(modelViewOrientation);
		break;
	case OF_MATRIX_PROJECTION:
		projection.preMult(m);
		uploadProjectionMatrix(projection);
		break;
	case OF_MATRIX_TEXTURE:
		textureMatrix.preMult(m);
		uploadTextureMatrix(textureMatrix);
		break;
	}
}

//----------------------------------------------------------
void ofProgrammableGLRenderer::multMatrix (const float *m){
	ofMatrix4x4 mat(m);
	multMatrix(mat);
}

//----------------------------------------------------------
void ofProgrammableGLRenderer::uploadModelViewMatrix(const ofMatrix4x4 & m){
	currentShader.setUniformMatrix4f("modelViewMatrix",m);
}

//----------------------------------------------------------
void ofProgrammableGLRenderer::uploadProjectionMatrix(const ofMatrix4x4 & m){
	currentShader.setUniformMatrix4f("projectionMatrix",m);
}

void ofProgrammableGLRenderer::uploadTextureMatrix(const ofMatrix4x4 & m){
	currentShader.setUniformMatrix4f("textureMatrix",m);
}

//----------------------------------------------------------
void ofProgrammableGLRenderer::setColor(const ofColor & color){
	setColor(color.r,color.g,color.b,color.a);
}

//----------------------------------------------------------
void ofProgrammableGLRenderer::setColor(const ofColor & color, int _a){
	setColor(color.r,color.g,color.b,_a);
}

//----------------------------------------------------------
void ofProgrammableGLRenderer::setColor(int _r, int _g, int _b){
	setColor(_r, _g, _b, 255);
}

//----------------------------------------------------------
void ofProgrammableGLRenderer::setColor(int _r, int _g, int _b, int _a){
	currentColor.set(_r/255.f,_g/255.f,_b/255.f,_a/255.f);
	currentShader.setUniform4f("color",currentColor.r,currentColor.g,currentColor.b,currentColor.a);
}

//----------------------------------------------------------
void ofProgrammableGLRenderer::setColor(int gray){
	setColor(gray, gray, gray);
}

//----------------------------------------------------------
void ofProgrammableGLRenderer::setHexColor(int hexColor){
	int r = (hexColor >> 16) & 0xff;
	int g = (hexColor >> 8) & 0xff;
	int b = (hexColor >> 0) & 0xff;
	setColor(r,g,b);
}

//----------------------------------------------------------
void ofProgrammableGLRenderer::clear(float r, float g, float b, float a) {
	glClearColor(r / 255., g / 255., b / 255., a / 255.);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

//----------------------------------------------------------
void ofProgrammableGLRenderer::clear(float brightness, float a) {
	clear(brightness, brightness, brightness, a);
}

//----------------------------------------------------------
void ofProgrammableGLRenderer::clearAlpha() {
	glColorMask(0, 0, 0, 1);
	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT);
	glColorMask(1, 1, 1, 1);
}

//----------------------------------------------------------
void ofProgrammableGLRenderer::setBackgroundAuto(bool bAuto){
	bBackgroundAuto = bAuto;
}

//----------------------------------------------------------
bool ofProgrammableGLRenderer::bClearBg(){
	return bBackgroundAuto;
}

//----------------------------------------------------------
ofFloatColor & ofProgrammableGLRenderer::getBgColor(){
	return bgColor;
}

//----------------------------------------------------------
void ofProgrammableGLRenderer::background(const ofColor & c){
	bgColor = c;
	glClearColor(bgColor[0],bgColor[1],bgColor[2], bgColor[3]);
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

//----------------------------------------------------------
void ofProgrammableGLRenderer::background(float brightness) {
	background(brightness);
}

//----------------------------------------------------------
void ofProgrammableGLRenderer::background(int hexColor, float _a){
	background ( (hexColor >> 16) & 0xff, (hexColor >> 8) & 0xff, (hexColor >> 0) & 0xff, _a);
}

//----------------------------------------------------------
void ofProgrammableGLRenderer::background(int r, int g, int b, int a){
	background(ofColor(r,g,b,a));
}

//----------------------------------------------------------
void ofProgrammableGLRenderer::setFillMode(ofFillFlag fill){
	bFilled = fill;
}

//----------------------------------------------------------
ofFillFlag ofProgrammableGLRenderer::getFillMode(){
	return bFilled;
}

//----------------------------------------------------------
void ofProgrammableGLRenderer::setRectMode(ofRectMode mode){
	rectMode = mode;
}

//----------------------------------------------------------
ofRectMode ofProgrammableGLRenderer::getRectMode(){
	return rectMode;
}

//----------------------------------------------------------
void ofProgrammableGLRenderer::setLineWidth(float lineWidth){
	glLineWidth(lineWidth);
}

//----------------------------------------------------------
void ofProgrammableGLRenderer::setLineSmoothing(bool smooth){
	bSmoothHinted = smooth;
}

//----------------------------------------------------------
void ofProgrammableGLRenderer::startSmoothing(){
    // TODO :: needs ES2 code.
}

//----------------------------------------------------------
void ofProgrammableGLRenderer::endSmoothing(){
    // TODO :: needs ES2 code.
}

//----------------------------------------------------------
void ofProgrammableGLRenderer::setBlendMode(ofBlendMode blendMode){
	switch (blendMode){

		case OF_BLENDMODE_ALPHA:{
			glEnable(GL_BLEND);
			#ifndef TARGET_OPENGLES
				glBlendEquation(GL_FUNC_ADD);
			#endif
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			break;
		}

		case OF_BLENDMODE_ADD:{
			glEnable(GL_BLEND);
			#ifndef TARGET_OPENGLES
				glBlendEquation(GL_FUNC_ADD);
			#endif
			glBlendFunc(GL_SRC_ALPHA, GL_ONE);
			break;
		}

		case OF_BLENDMODE_MULTIPLY:{
			glEnable(GL_BLEND);
			#ifndef TARGET_OPENGLES
				glBlendEquation(GL_FUNC_ADD);
			#endif
			glBlendFunc(GL_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA /* GL_ZERO or GL_ONE_MINUS_SRC_ALPHA */);
			break;
		}

		case OF_BLENDMODE_SCREEN:{
			glEnable(GL_BLEND);
			#ifndef TARGET_OPENGLES
				glBlendEquation(GL_FUNC_ADD);
			#endif
			glBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ONE);
			break;
		}

		case OF_BLENDMODE_SUBTRACT:{
			glEnable(GL_BLEND);
		#ifndef TARGET_OPENGLES
			glBlendEquation(GL_FUNC_REVERSE_SUBTRACT);
		#else
			ofLog(OF_LOG_WARNING, "OF_BLENDMODE_SUBTRACT not currently supported on OpenGL/ES");
		#endif
			glBlendFunc(GL_SRC_ALPHA, GL_ONE);
			break;
		}


		default:
			break;
	}
}

//----------------------------------------------------------
void ofProgrammableGLRenderer::enablePointSprites(){
    // TODO :: needs ES2 code.
}

//----------------------------------------------------------
void ofProgrammableGLRenderer::disablePointSprites(){
    // TODO :: needs ES2 code.
}

//----------------------------------------------------------
inline GLint ofProgrammableGLRenderer::getAttrLocationPosition(){
	return currentShader.getAttributeLocation("position");
}

//----------------------------------------------------------
inline GLint ofProgrammableGLRenderer::getAttrLocationColor(){
	return currentShader.getAttributeLocation("color");
}

//----------------------------------------------------------
inline GLint ofProgrammableGLRenderer::getAttrLocationNormal(){
	return currentShader.getAttributeLocation("normal");
}

//----------------------------------------------------------
inline GLint ofProgrammableGLRenderer::getAttrLocationTexCoord(){
	return currentShader.getAttributeLocation("texcoord");
}

//----------------------------------------------------------
inline ofShader & ofProgrammableGLRenderer::getCurrentShader(){
	return currentShader;
}

//----------------------------------------------------------
inline void ofProgrammableGLRenderer::setDefaultShader(ofShader & shader){
	defaultShader = shader;
}

//----------------------------------------------------------
inline void ofProgrammableGLRenderer::enableVertices(){
	glEnableVertexAttribArray(getAttrLocationPosition());
}

//----------------------------------------------------------
inline void ofProgrammableGLRenderer::enableTexCoords(){
	glEnableVertexAttribArray(getAttrLocationTexCoord());
	currentShader.setUniform1f("useTexture",1);
	texCoordsEnabled = true;
}

//----------------------------------------------------------
inline void ofProgrammableGLRenderer::enableColors(){
	glEnableVertexAttribArray(getAttrLocationColor());
	currentShader.setUniform1f("useColors",1);
	colorsEnabled = true;
}

//----------------------------------------------------------
inline void ofProgrammableGLRenderer::enableNormals(){
	glEnableVertexAttribArray(getAttrLocationNormal());
	normalsEnabled = true;
}

inline void ofProgrammableGLRenderer::disableVertices(){
	glDisableVertexAttribArray(getAttrLocationPosition());
}

//----------------------------------------------------------
inline void ofProgrammableGLRenderer::disableTexCoords(){
	glDisableVertexAttribArray(getAttrLocationTexCoord());
	currentShader.setUniform1f("useTexture",0);
	texCoordsEnabled = false;
}

//----------------------------------------------------------
inline void ofProgrammableGLRenderer::disableColors(){
	glDisableVertexAttribArray(getAttrLocationColor());
	currentShader.setUniform1f("useColors",0);
	colorsEnabled = false;
}

//----------------------------------------------------------
inline void ofProgrammableGLRenderer::disableNormals(){
	normalsEnabled = false;
}

//----------------------------------------------------------
void ofProgrammableGLRenderer::beginCustomShader(ofShader & shader){
	shader.setUniform1f("useTexture",texCoordsEnabled);
	shader.setUniform1f("useColors",colorsEnabled);
	shader.setUniform4f("color",currentColor.r,currentColor.g,currentColor.b,currentColor.a);
	shader.setUniformMatrix4f("modelViewMatrix",modelViewOrientation);
	shader.setUniformMatrix4f("projectionMatrix",projection);
	shader.setUniformMatrix4f("textureMatrix",textureMatrix);
	currentShader = shader;
}

//----------------------------------------------------------
void ofProgrammableGLRenderer::endCustomShader(){
	defaultShader.begin();
}
// ----------------------------------------------------------------------
#pragma mark- Primitive Draw Methods
// ----------------------------------------------------------------------

inline void ofProgrammableGLRenderer::preparePrimitiveDraw(ofVbo& vbo_){
	glBindBuffer(GL_ARRAY_BUFFER, vbo_.getVertId()); // bind to triangle vertices
	glEnableVertexAttribArray(currentShader.getAttributeLocation("position"));	// activate attribute 'position' in shader
	glVertexAttribPointer(currentShader.getAttributeLocation("position"), 3, GL_FLOAT,GL_FALSE,0,0);
	
}

inline void ofProgrammableGLRenderer::finishPrimitiveDraw(){
	// ubind, basically.
	glDisableVertexAttribArray(0);			// disable vertex attrib array.
	glBindBuffer(GL_ARRAY_BUFFER,0);		// unbind by binding to zero
}

//----------------------------------------------------------
void ofProgrammableGLRenderer::drawLine(float x1, float y1, float z1, float x2, float y2, float z2){
	linePoints[0].set(x1,y1,z1);
	linePoints[1].set(x2,y2,z2);
    
	// use smoothness, if requested:
	if (bSmoothHinted) startSmoothing();

	disableTexCoords();
	disableColors();
    
	lineVbo.setVertexData(&linePoints[0], 2, GL_DYNAMIC_DRAW);
	preparePrimitiveDraw(lineVbo);
	glDrawArrays(GL_LINES, 0, 2);
	finishPrimitiveDraw();
    
	// use smoothness, if requested:
	if (bSmoothHinted) endSmoothing();
}

//----------------------------------------------------------
void ofProgrammableGLRenderer::drawRectangle(float x, float y, float z, float w, float h) {
	if (rectMode == OF_RECTMODE_CORNER){
		rectPoints[0].set(x,y,z);
		rectPoints[1].set(x+w, y, z);
		rectPoints[2].set(x+w, y+h, z);
		rectPoints[3].set(x, y+h, z);
	}else{
		rectPoints[0].set(x-w/2.0f, y-h/2.0f, z);
		rectPoints[1].set(x+w/2.0f, y-h/2.0f, z);
		rectPoints[2].set(x+w/2.0f, y+h/2.0f, z);
		rectPoints[3].set(x-w/2.0f, y+h/2.0f, z);
	}
    
	// use smoothness, if requested:
	if (bSmoothHinted && bFilled == OF_OUTLINE) startSmoothing();

	disableTexCoords();
	disableColors();

	rectVbo.setVertexData(&rectPoints[0], 4, GL_DYNAMIC_DRAW);
	
	preparePrimitiveDraw(rectVbo);
	glDrawArrays((bFilled == OF_FILLED) ? GL_TRIANGLE_FAN : GL_LINE_LOOP, 0, 4);
	finishPrimitiveDraw();
    
	// use smoothness, if requested:
	if (bSmoothHinted && bFilled == OF_OUTLINE) endSmoothing();
}

//----------------------------------------------------------
void ofProgrammableGLRenderer::drawTriangle(float x1, float y1, float z1, float x2, float y2, float z2, float x3, float y3, float z3){
	triPoints[0].set(x1,y1,z1);
	triPoints[1].set(x2,y2,z2);
	triPoints[2].set(x3,y3,z3);
    
	// use smoothness, if requested:
	if (bSmoothHinted && bFilled == OF_OUTLINE) startSmoothing();

	disableTexCoords();
	disableColors();
    
	triangleVbo.setVertexData(&triPoints[0], 3, GL_DYNAMIC_DRAW);
	
	preparePrimitiveDraw(triangleVbo);
	glDrawArrays((bFilled == OF_FILLED) ? GL_TRIANGLE_STRIP : GL_LINE_LOOP, 0, 3);
	finishPrimitiveDraw();
    
	// use smoothness, if requested:
	if (bSmoothHinted && bFilled == OF_OUTLINE) endSmoothing();
}

//----------------------------------------------------------
void ofProgrammableGLRenderer::drawCircle(float x, float y, float z,  float radius){
	vector<ofPoint> & circleCache = circlePolyline.getVertices();
	for(int i=0;i<(int)circleCache.size();i++){
		circlePoints[i].set(radius*circleCache[i].x+x,radius*circleCache[i].y+y,z);
	}
    
	// use smoothness, if requested:
	if (bSmoothHinted && bFilled == OF_OUTLINE) startSmoothing();

	circleVbo.setVertexData(&circlePoints[0].x, 3, circlePoints.size(), GL_DYNAMIC_DRAW, sizeof(ofVec3f));
	
	disableTexCoords();
	disableColors();
    
	preparePrimitiveDraw(circleVbo);
	glDrawArrays((bFilled == OF_FILLED) ? GL_TRIANGLE_FAN : GL_LINE_STRIP, 0, circlePoints.size());
    finishPrimitiveDraw();
	
	// use smoothness, if requested:
	if (bSmoothHinted && bFilled == OF_OUTLINE) endSmoothing();
}

//----------------------------------------------------------
void ofProgrammableGLRenderer::drawSphere(float x, float y, float z, float radius) {
    glEnable(GL_NORMALIZE);
    pushMatrix();
    scale(radius, radius, radius);
    if(bFilled) {
        sphereMesh.draw();
    } else {
        sphereMesh.drawWireframe();
    }
    popMatrix();
    glDisable(GL_NORMALIZE);
}

//----------------------------------------------------------
void ofProgrammableGLRenderer::drawEllipse(float x, float y, float z, float width, float height){
	float radiusX = width*0.5;
	float radiusY = height*0.5;
	vector<ofPoint> & circleCache = circlePolyline.getVertices();
	for(int i=0;i<(int)circleCache.size();i++){
		circlePoints[i].set(radiusX*circlePolyline[i].x+x,radiusY*circlePolyline[i].y+y,z);
	}
    
	// use smoothness, if requested:
	if (bSmoothHinted && bFilled == OF_OUTLINE) startSmoothing();

	circleVbo.setVertexData(&circlePoints[0].x, 3, circlePoints.size(), GL_DYNAMIC_DRAW, sizeof(ofVec3f));
	
	disableTexCoords();
	disableColors();
    
	preparePrimitiveDraw(circleVbo);
	glDrawArrays((bFilled == OF_FILLED) ? GL_TRIANGLE_FAN : GL_LINE_STRIP, 0, circlePoints.size());
    finishPrimitiveDraw();
    
	// use smoothness, if requested:
	if (bSmoothHinted && bFilled == OF_OUTLINE) endSmoothing();
}

//----------------------------------------------------------
void ofProgrammableGLRenderer::drawString(string textString, float x, float y, float z, ofDrawBitmapMode mode){
	// this is copied from the ofTrueTypeFont
	//GLboolean blend_enabled = glIsEnabled(GL_BLEND); //TODO: this is not used?
	GLint blend_src, blend_dst;
	glGetIntegerv( GL_BLEND_SRC, &blend_src );
	glGetIntegerv( GL_BLEND_DST, &blend_dst );

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


	int len = (int)textString.length();
	//float yOffset = 0;
	float fontSize = 8.0f;
	bool bOrigin = false;

	float sx = 0;
	float sy = -fontSize;


	///////////////////////////
	// APPLY TRANSFORM / VIEW
	///////////////////////////
	//

	bool hasModelView = false;
	bool hasProjection = false;
	bool hasViewport = false;

	ofRectangle rViewport;

#ifdef TARGET_OPENGLES
	if(mode == OF_BITMAPMODE_MODEL_BILLBOARD) {
		mode = OF_BITMAPMODE_SIMPLE;
	}
#endif

	switch (mode) {

		case OF_BITMAPMODE_SIMPLE:

			sx += x;
			sy += y;
			break;

		case OF_BITMAPMODE_SCREEN:

			hasViewport = true;
			pushView();

			rViewport = ofGetWindowRect();
			viewport(rViewport);

			matrixMode(OF_MATRIX_PROJECTION);
			loadIdentityMatrix();
			matrixMode(OF_MATRIX_MODELVIEW);
			loadIdentityMatrix();

			translate(-1, 1, 0);
			scale(2/rViewport.width, -2/rViewport.height, 1);

			translate(x, y, 0);
			break;

		case OF_BITMAPMODE_VIEWPORT:

			rViewport = getCurrentViewport();

			hasProjection = true;
			matrixMode(OF_MATRIX_PROJECTION);
			pushMatrix();
			loadIdentityMatrix();

			hasModelView = true;
			matrixMode(OF_MATRIX_MODELVIEW);
			pushMatrix();
			loadIdentityMatrix();

			translate(-1, 1, 0);
			scale(2/rViewport.width, -2/rViewport.height, 1);

			translate(x, y, 0);
			break;

		case OF_BITMAPMODE_MODEL:

			hasModelView = true;
			matrixMode(OF_MATRIX_MODELVIEW);
			pushMatrix();

			translate(x, y, z);
			scale(1, -1, 0);
			break;

		case OF_BITMAPMODE_MODEL_BILLBOARD:
			//our aim here is to draw to screen
			//at the viewport position related
			//to the world position x,y,z

			// ***************
			// this will not compile for opengl ES
			// ***************
#ifndef TARGET_OPENGLES
			//gluProject method
			GLdouble modelview[16], projection[16];
			GLint view[4];
			double dScreenX, dScreenY, dScreenZ;
			glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
			glGetDoublev(GL_PROJECTION_MATRIX, projection);
			glGetIntegerv(GL_VIEWPORT, view);
			view[0] = 0; view[1] = 0; //we're already drawing within viewport
			gluProject(x, y, z, modelview, projection, view, &dScreenX, &dScreenY, &dScreenZ);

			if (dScreenZ >= 1)
				return;

			rViewport = ofGetCurrentViewport();

			hasProjection = true;
			glMatrixMode(GL_PROJECTION);
			glPushMatrix();
			glLoadIdentity();

			hasModelView = true;
			glMatrixMode(GL_MODELVIEW);
			glPushMatrix();
			glLoadIdentity();

			glTranslatef(-1, -1, 0);
			glScalef(2/rViewport.width, 2/rViewport.height, 1);

			glTranslatef(dScreenX, dScreenY, 0);

			if(currentFbo == NULL) {
				glScalef(1, -1, 1);
			} else {
				glScalef(1,  1, 1); // invert when rendering inside an fbo
			}

#endif
			break;

		default:
			break;
	}
	//
	///////////////////////////


	// (c) enable texture once before we start drawing each char (no point turning it on and off constantly)
	//We do this because its way faster
	ofDrawBitmapCharacterStart(textString.size());

	for(int c = 0; c < len; c++){
		if(textString[c] == '\n'){

			sy += bOrigin ? -1 : 1 * (fontSize*1.7);
			if(mode == OF_BITMAPMODE_SIMPLE) {
				sx = x;
			} else {
				sx = 0;
			}

			//glRasterPos2f(x,y + (int)yOffset);
		} else if (textString[c] >= 32){
			// < 32 = control characters - don't draw
			// solves a bug with control characters
			// getting drawn when they ought to not be
			ofDrawBitmapCharacter(textString[c], (int)sx, (int)sy);

			sx += fontSize;
		}
	}
	//We do this because its way faster
	ofDrawBitmapCharacterEnd();


	if (hasModelView)
		popMatrix();

	if (hasProjection)
	{
		matrixMode(OF_MATRIX_PROJECTION);
		popMatrix();
		matrixMode(OF_MATRIX_MODELVIEW);
	}

	if (hasViewport)
		popView();

	glBlendFunc(blend_src, blend_dst);
}
