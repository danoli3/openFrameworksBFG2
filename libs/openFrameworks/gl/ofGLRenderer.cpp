#include "ofGLRenderer.h"
#include "ofMesh.h"
#include "ofPath.h"
#include "ofMesh.h"
#include "of3dPrimitives.h"
#include "ofBitmapFont.h"
#include "ofGLUtils.h"
#include "ofImage.h"
#include "ofFbo.h"
#include "ofLight.h"
#include "ofMaterial.h"
#include "ofCamera.h"
#include "ofTrueTypeFont.h"
#include "ofNode.h"

const string ofGLRenderer::TYPE="GL";

//----------------------------------------------------------
ofGLRenderer::ofGLRenderer(const ofAppBaseWindow * _window)
:matrixStack(_window)
,graphics3d(this){
	bBackgroundAuto = true;

	linePoints.resize(2);
	rectPoints.resize(4);
	triPoints.resize(3);
	normalsEnabled = false;
	lightingEnabled = false;
	alphaMaskTextureTarget = GL_TEXTURE_2D;
	window = _window;
	currentFramebufferId = 0;
	defaultFramebufferId = 0;
}

void ofGLRenderer::setup(){
	setupGraphicDefaults();
	viewport();
	setupScreenPerspective();
}

void ofGLRenderer::startRender(){
#ifdef TARGET_OPENGLES
	// OpenGL ES might have set a default frame buffer for
	// MSAA rendering to the window, bypassing ofFbo, so we
	// can't trust ofFbo to have correctly tracked the bind
	// state. Therefore, we are forced to use the slower glGet() method
	// to be sure to get the correct default framebuffer.
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &defaultFramebufferId);
#endif
	currentFramebufferId = defaultFramebufferId;
	framebufferIdStack.push_back(defaultFramebufferId);
    matrixStack.setRenderSurface(*window);
	viewport();
    // to do non auto clear on PC for now - we do something like "single" buffering --
    // it's not that pretty but it work for the most part

    #ifdef TARGET_WIN32
    if (getBackgroundAuto() == false){
        glDrawBuffer (GL_FRONT);
    }
    #endif

	if ( getBackgroundAuto() ){// || ofGetFrameNum() < 3){
		background(currentStyle.bgColor);
	}
}

void ofGLRenderer::finishRender(){
	matrixStack.clearStacks();
	framebufferIdStack.clear();
}

//----------------------------------------------------------
void ofGLRenderer::draw(const ofMesh & vertexData, ofPolyRenderMode renderType, bool useColors, bool useTextures, bool useNormals) const{
		if (currentStyle.smoothing) const_cast<ofGLRenderer*>(this)->startSmoothing();
#ifndef TARGET_OPENGLES
		glPolygonMode(GL_FRONT_AND_BACK, ofGetGLPolyMode(renderType));
		if(vertexData.getNumVertices()){
			glEnableClientState(GL_VERTEX_ARRAY);
			glVertexPointer(3, GL_FLOAT, sizeof(ofVec3f), &vertexData.getVerticesPointer()->x);
		}
		if(vertexData.getNumNormals() && useNormals){
			glEnableClientState(GL_NORMAL_ARRAY);
			glNormalPointer(GL_FLOAT, sizeof(ofVec3f), &vertexData.getNormalsPointer()->x);
		}
		if(vertexData.getNumColors() && useColors){
			glEnableClientState(GL_COLOR_ARRAY);
			glColorPointer(4,GL_FLOAT, sizeof(ofFloatColor), &vertexData.getColorsPointer()->r);
		}

		if(vertexData.getNumTexCoords() && useTextures){
			set<int>::iterator textureLocation = textureLocationsEnabled.begin();
			for(;textureLocation!=textureLocationsEnabled.end();textureLocation++){
				glActiveTexture(GL_TEXTURE0+*textureLocation);
				glClientActiveTexture(GL_TEXTURE0+*textureLocation);
				glEnableClientState(GL_TEXTURE_COORD_ARRAY);
				glTexCoordPointer(2, GL_FLOAT, sizeof(ofVec2f), &vertexData.getTexCoordsPointer()->x);
			}
			glActiveTexture(GL_TEXTURE0);
			glClientActiveTexture(GL_TEXTURE0);
		}

		if(vertexData.getNumIndices()){
	#ifdef TARGET_OPENGLES
			glDrawElements(ofGetGLPrimitiveMode(vertexData.getMode()), vertexData.getNumIndices(),GL_UNSIGNED_SHORT,vertexData.getIndexPointer());
	#else
			glDrawElements(ofGetGLPrimitiveMode(vertexData.getMode()), vertexData.getNumIndices(),GL_UNSIGNED_INT,vertexData.getIndexPointer());
	#endif
		}else{
			glDrawArrays(ofGetGLPrimitiveMode(vertexData.getMode()), 0, vertexData.getNumVertices());
		}

		if(vertexData.getNumColors() && useColors){
			glDisableClientState(GL_COLOR_ARRAY);
		}
		if(vertexData.getNumNormals() && useNormals){
			glDisableClientState(GL_NORMAL_ARRAY);
		}
		if(vertexData.getNumTexCoords() && useTextures){
			glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		}
		glPolygonMode(GL_FRONT_AND_BACK, currentStyle.bFill ?  GL_FILL : GL_LINE);
#else
		if(vertexData.getNumVertices()){
			glEnableClientState(GL_VERTEX_ARRAY);
			glVertexPointer(3, GL_FLOAT, sizeof(ofVec3f), vertexData.getVerticesPointer());
		}
		if(vertexData.getNumNormals() && useNormals){
			glEnableClientState(GL_NORMAL_ARRAY);
			glNormalPointer(GL_FLOAT, 0, vertexData.getNormalsPointer());
		}
		if(vertexData.getNumColors() && useColors){
			glEnableClientState(GL_COLOR_ARRAY);
			glColorPointer(4,GL_FLOAT, sizeof(ofFloatColor), vertexData.getColorsPointer());
		}

		if(vertexData.getNumTexCoords() && useTextures){
			set<int>::iterator textureLocation = textureLocationsEnabled.begin();
			for(;textureLocation!=textureLocationsEnabled.end();textureLocation++){
				glActiveTexture(GL_TEXTURE0+*textureLocation);
				glClientActiveTexture(GL_TEXTURE0+*textureLocation);
				glEnableClientState(GL_TEXTURE_COORD_ARRAY);
				glTexCoordPointer(2, GL_FLOAT, sizeof(ofVec2f), &vertexData.getTexCoordsPointer()->x);
			}
			glActiveTexture(GL_TEXTURE0);
			glClientActiveTexture(GL_TEXTURE0);
		}

		GLenum drawMode;
		switch(renderType){
		case OF_MESH_POINTS:
			drawMode = GL_POINTS;
			break;
		case OF_MESH_WIREFRAME:
			drawMode = GL_LINES;
			break;
		case OF_MESH_FILL:
			drawMode = ofGetGLPrimitiveMode(vertexData.getMode());
			break;
		default:
			drawMode = ofGetGLPrimitiveMode(vertexData.getMode());
			break;
		}

		if(vertexData.getNumIndices()){
			glDrawElements(drawMode, vertexData.getNumIndices(),GL_UNSIGNED_SHORT,vertexData.getIndexPointer());
		}else{
			glDrawArrays(drawMode, 0, vertexData.getNumVertices());
		}
		if(vertexData.getNumColors() && useColors){
			glDisableClientState(GL_COLOR_ARRAY);
		}
		if(vertexData.getNumNormals() && useNormals){
			glDisableClientState(GL_NORMAL_ARRAY);
		}
		if(vertexData.getNumTexCoords() && useTextures){
			glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		}
#endif
		if (currentStyle.smoothing) const_cast<ofGLRenderer*>(this)->endSmoothing();
}

//----------------------------------------------------------
void ofGLRenderer::draw(const ofVboMesh & mesh, ofPolyRenderMode renderType) const{
	drawInstanced(mesh,renderType,1);
}

//----------------------------------------------------------
void ofGLRenderer::drawInstanced(const ofVboMesh & mesh, ofPolyRenderMode renderType, int primCount) const{
	if(mesh.getNumVertices()==0) return;
	GLuint mode = ofGetGLPrimitiveMode(mesh.getMode());
#ifndef TARGET_OPENGLES
	glPolygonMode(GL_FRONT_AND_BACK, ofGetGLPolyMode(renderType));
	if(mesh.getNumIndices() && renderType!=OF_MESH_POINTS){
		if (primCount <= 1) {
			drawElements(mesh.getVbo(),mode,mesh.getNumIndices());
		} else {
			drawElementsInstanced(mesh.getVbo(),mode,mesh.getNumIndices(),primCount);
		}
	}else{
		if (primCount <= 1) {
			draw(mesh.getVbo(),mode,0,mesh.getNumVertices());
		} else {
			drawInstanced(mesh.getVbo(),mode,0,mesh.getNumVertices(),primCount);
		}
	}
	glPolygonMode(GL_FRONT_AND_BACK, currentStyle.bFill ?  GL_FILL : GL_LINE);
#else
	if(renderType == OF_MESH_POINTS){
		draw(mesh.getVbo(),GL_POINTS,0,mesh.getNumVertices());
	}else if(renderType == OF_MESH_WIREFRAME){
		if(mesh.getNumIndices()){
			drawElements(mesh.getVbo(),GL_LINES,mesh.getNumIndices());
		}else{
			draw(mesh.getVbo(),GL_LINES,0,mesh.getNumVertices());
		}
	}else{
		if(mesh.getNumIndices()){
			drawElements(mesh.getVbo(),mode,mesh.getNumIndices());
		}else{
			draw(mesh.getVbo(),mode,0,mesh.getNumVertices());
		}
	}
#endif
}

//----------------------------------------------------------
void ofGLRenderer::draw( const of3dPrimitive& model, ofPolyRenderMode renderType)  const{
	const_cast<ofGLRenderer*>(this)->pushMatrix();
	const_cast<ofGLRenderer*>(this)->multMatrix(model.getGlobalTransformMatrix());
	if(model.isUsingVbo()){
		draw(static_cast<const ofVboMesh&>(model.getMesh()),renderType);
	}else{
		draw(model.getMesh(),renderType);
	}
	const_cast<ofGLRenderer*>(this)->popMatrix();
}

//----------------------------------------------------------
void ofGLRenderer::draw(const ofNode& node) const{
	const_cast<ofGLRenderer*>(this)->pushMatrix();
	const_cast<ofGLRenderer*>(this)->multMatrix(node.getGlobalTransformMatrix());
	node.customDraw(this);
	const_cast<ofGLRenderer*>(this)->popMatrix();
}

//----------------------------------------------------------
void ofGLRenderer::draw(const ofPolyline & poly) const{
	if(!poly.getVertices().empty()) {
		// use smoothness, if requested:
		if (currentStyle.smoothing) const_cast<ofGLRenderer*>(this)->startSmoothing();

		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(3, GL_FLOAT, sizeof(ofVec3f), &poly.getVertices()[0].x);
		glDrawArrays(poly.isClosed()?GL_LINE_LOOP:GL_LINE_STRIP, 0, poly.size());

		// use smoothness, if requested:
		if (currentStyle.smoothing) const_cast<ofGLRenderer*>(this)->endSmoothing();
	}
}

//----------------------------------------------------------
void ofGLRenderer::draw(const ofPath & shape) const{
	ofColor prevColor;
	if(shape.getUseShapeColor()){
		prevColor = currentStyle.color;
	}
	ofGLRenderer * mut_this = const_cast<ofGLRenderer*>(this);
	if(shape.isFilled()){
		const ofMesh & mesh = shape.getTessellation();
		if(shape.getUseShapeColor()){
			mut_this->setColor( shape.getFillColor(),shape.getFillColor().a);
		}
		draw(mesh,OF_MESH_FILL);
	}
	if(shape.hasOutline()){
		float lineWidth = currentStyle.lineWidth;
		if(shape.getUseShapeColor()){
			mut_this->setColor( shape.getStrokeColor(), shape.getStrokeColor().a);
		}
		mut_this->setLineWidth( shape.getStrokeWidth() );
		const vector<ofPolyline> & outlines = shape.getOutline();
		for(int i=0; i<(int)outlines.size(); i++)
			draw(outlines[i]);
		mut_this->setLineWidth(lineWidth);
	}
	if(shape.getUseShapeColor()){
		mut_this->setColor(prevColor);
	}
}

//----------------------------------------------------------
void ofGLRenderer::draw(const ofImage & image, float x, float y, float z, float w, float h, float sx, float sy, float sw, float sh) const{
	if(image.isUsingTexture()){
		const ofTexture& tex = image.getTexture();
		if(tex.isAllocated()) {
			const_cast<ofGLRenderer*>(this)->bind(tex,0);
			draw(tex.getMeshForSubsection(x,y,z,w,h,sx,sy,sw,sh,isVFlipped(),currentStyle.rectMode),OF_MESH_FILL,false,true,false);
			const_cast<ofGLRenderer*>(this)->unbind(tex,0);
		} else {
			ofLogWarning("ofGLRenderer") << "drawing an unallocated texture";
		}
	}
}

//----------------------------------------------------------
void ofGLRenderer::draw(const ofFloatImage & image, float x, float y, float z, float w, float h, float sx, float sy, float sw, float sh) const{
	if(image.isUsingTexture()){
		const ofTexture& tex = image.getTexture();
		if(tex.isAllocated()) {
			const_cast<ofGLRenderer*>(this)->bind(tex,0);
			draw(tex.getMeshForSubsection(x,y,z,w,h,sx,sy,sw,sh,isVFlipped(),currentStyle.rectMode),OF_MESH_FILL,false,true,false);
			const_cast<ofGLRenderer*>(this)->unbind(tex,0);
		} else {
			ofLogWarning("ofGLRenderer") << "draw(): texture is not allocated";
		}
	}
}

//----------------------------------------------------------
void ofGLRenderer::draw(const ofShortImage & image, float x, float y, float z, float w, float h, float sx, float sy, float sw, float sh) const{
	if(image.isUsingTexture()){
		const ofTexture& tex = image.getTexture();
		if(tex.isAllocated()) {
			const_cast<ofGLRenderer*>(this)->bind(tex,0);
			draw(tex.getMeshForSubsection(x,y,z,w,h,sx,sy,sw,sh,isVFlipped(),currentStyle.rectMode),OF_MESH_FILL,false,true,false);
			const_cast<ofGLRenderer*>(this)->unbind(tex,0);
		} else {
			ofLogWarning("ofGLRenderer") << "draw(): texture is not allocated";
		}
	}
}

//----------------------------------------------------------
void ofGLRenderer::draw(const ofTexture & tex, float x, float y, float z, float w, float h, float sx, float sy, float sw, float sh) const{
	if(tex.isAllocated()) {
		const_cast<ofGLRenderer*>(this)->bind(tex,0);
		draw(tex.getMeshForSubsection(x,y,z,w,h,sx,sy,sw,sh,isVFlipped(),currentStyle.rectMode),OF_MESH_FILL,false,true,false);
		const_cast<ofGLRenderer*>(this)->unbind(tex,0);
	} else {
		ofLogWarning("ofGLRenderer") << "draw(): texture is not allocated";
	}
}

//----------------------------------------------------------
void ofGLRenderer::draw(const ofBaseVideoDraws & video, float x, float y, float w, float h) const{
	if(video.isInitialized() && video.isUsingTexture()){
		const_cast<ofGLRenderer*>(this)->bind(video);
		draw(video.getTexture().getMeshForSubsection(x,y,0,w,h,0,0,video.getWidth(),video.getHeight(),isVFlipped(),currentStyle.rectMode),OF_MESH_FILL,false,true,false);
		const_cast<ofGLRenderer*>(this)->unbind(video);
	}
}

//----------------------------------------------------------
void ofGLRenderer::draw(const ofVbo & vbo, GLuint drawMode, int first, int total) const{
	if(vbo.getUsingVerts()) {
		vbo.bind();
		glDrawArrays(drawMode, first, total);
		vbo.unbind();
	}
}

//----------------------------------------------------------
void ofGLRenderer::drawElements(const ofVbo & vbo, GLuint drawMode, int amt) const{
	if(vbo.getUsingVerts()) {
		vbo.bind();
#ifdef TARGET_OPENGLES
        glDrawElements(drawMode, amt, GL_UNSIGNED_SHORT, NULL);
#else
        glDrawElements(drawMode, amt, GL_UNSIGNED_INT, NULL);
#endif
		vbo.unbind();
	}
}

//----------------------------------------------------------
void ofGLRenderer::drawInstanced(const ofVbo & vbo, GLuint drawMode, int first, int total, int primCount) const{
	if(vbo.getUsingVerts()) {
		vbo.bind();
#ifdef TARGET_OPENGLES
		// todo: activate instancing once OPENGL ES supports instancing, starting with version 3.0
		// unfortunately there is currently no easy way within oF to query the current OpenGL version.
		// https://www.khronos.org/opengles/sdk/docs/man3/xhtml/glDrawElementsInstanced.xml
		ofLogWarning("ofVbo") << "drawInstanced(): hardware instancing is not supported on OpenGL ES < 3.0";
		// glDrawArraysInstanced(drawMode, first, total, primCount);
#else
		glDrawArraysInstanced(drawMode, first, total, primCount);
#endif
		vbo.unbind();
	}
}

//----------------------------------------------------------
void ofGLRenderer::drawElementsInstanced(const ofVbo & vbo, GLuint drawMode, int amt, int primCount) const{
	if(vbo.getUsingVerts()) {
		vbo.bind();
#ifdef TARGET_OPENGLES
        // todo: activate instancing once OPENGL ES supports instancing, starting with version 3.0
        // unfortunately there is currently no easy way within oF to query the current OpenGL version.
        // https://www.khronos.org/opengles/sdk/docs/man3/xhtml/glDrawElementsInstanced.xml
        ofLogWarning("ofVbo") << "drawElementsInstanced(): hardware instancing is not supported on OpenGL ES < 3.0";
        // glDrawElementsInstanced(drawMode, amt, GL_UNSIGNED_SHORT, NULL, primCount);
#else
        glDrawElementsInstanced(drawMode, amt, GL_UNSIGNED_INT, NULL, primCount);
#endif
		vbo.unbind();
	}
}

//----------------------------------------------------------
ofPath & ofGLRenderer::getPath(){
	return path;
}

//----------------------------------------------------------
void ofGLRenderer::bind(const ofBaseVideoDraws & video){
	if(video.isInitialized() && video.isUsingTexture()){
		bind(video.getTexture(),0);
	}
}

//----------------------------------------------------------
void ofGLRenderer::unbind(const ofBaseVideoDraws & video){
	if(video.isInitialized() && video.isUsingTexture()){
		video.getTexture().unbind();
	}
}

//----------------------------------------------------------
void ofGLRenderer::bind(const ofShader & shader){
	glUseProgram(shader.getProgram());
}

//----------------------------------------------------------
void ofGLRenderer::unbind(const ofShader & shader){
	glUseProgram(0);
}


//----------------------------------------------------------
void ofGLRenderer::begin(const ofFbo & fbo, bool setupPerspective){
	pushView();
	pushStyle();
	matrixStack.setRenderSurface(fbo);
	viewport();
	if(setupPerspective){
		setupScreenPerspective();
	}else{
		ofMatrix4x4 m;
		glGetFloatv(GL_PROJECTION_MATRIX,m.getPtr());
		m =  m*matrixStack.getOrientationMatrixInverse();
		ofMatrixMode currentMode = matrixStack.getCurrentMatrixMode();
		matrixStack.matrixMode(OF_MATRIX_PROJECTION);
		matrixStack.loadMatrix(m.getPtr());
		glMatrixMode(GL_PROJECTION);
		glLoadMatrixf(matrixStack.getProjectionMatrix().getPtr());
		matrixMode(currentMode);
	}
	bind(fbo);
}

//----------------------------------------------------------
void ofGLRenderer::end(const ofFbo & fbo){
	unbind(fbo);
	matrixStack.setRenderSurface(*window);
	popStyle();
	popView();
}

//----------------------------------------------------------
void ofGLRenderer::bind(const ofFbo & fbo){
	if (currentFramebufferId == fbo.getId()){
		ofLogWarning() << "Framebuffer with id:" << " cannot be bound onto itself. \n" <<
			"Most probably you forgot to end() the current framebuffer before calling begin() again.";
		return;
	}
	// this method could just as well have been placed in ofBaseGLRenderer
	// and shared over both programmable and fixed function renderer.
	// I'm keeping it here, so that if we want to do more fancyful
	// named framebuffers with GL 4.5+, we can have
	// different implementations.
	framebufferIdStack.push_back(currentFramebufferId);
	currentFramebufferId = fbo.getId();
	glBindFramebuffer(GL_FRAMEBUFFER, currentFramebufferId);
}

//----------------------------------------------------------
void ofGLRenderer::bindForBlitting(const ofFbo & fboSrc, ofFbo & fboDst, int attachmentPoint){
	if (currentFramebufferId == fboSrc.getId()){
		ofLogWarning() << "Framebuffer with id:" << " cannot be bound onto itself. \n" <<
			"Most probably you forgot to end() the current framebuffer before calling getTexture().";
		return;
	}
	// this method could just as well have been placed in ofBaseGLRenderer
	// and shared over both programmable and fixed function renderer.
	// I'm keeping it here, so that if we want to do more fancyful
	// named framebuffers with GL 4.5+, we can have
	// different implementations.
	framebufferIdStack.push_back(currentFramebufferId);
	currentFramebufferId = fboSrc.getId();
	glBindFramebuffer(GL_READ_FRAMEBUFFER, currentFramebufferId);
	glReadBuffer(GL_COLOR_ATTACHMENT0 + attachmentPoint);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fboDst.getIdDrawBuffer()());
	glDrawBuffer(GL_COLOR_ATTACHMENT0 + attachmentPoint);
}

//----------------------------------------------------------
void ofGLRenderer::unbind(const ofFbo & fbo){
	if(framebufferIdStack.empty()){
		ofLogError() << "unbalanced fbo bind/unbind binding default framebuffer";
		currentFramebufferId = defaultFramebufferId;
	}else{
		currentFramebufferId = framebufferIdStack.back();
		framebufferIdStack.pop_back();
	}
	glBindFramebuffer(GL_FRAMEBUFFER, currentFramebufferId);
	fbo.flagDirty();
}

//----------------------------------------------------------
void ofGLRenderer::bind(const ofBaseMaterial & material){
	ofFloatColor diffuse = material.getDiffuseColor();
	ofFloatColor specular = material.getSpecularColor();
	ofFloatColor ambient = material.getAmbientColor();
	ofFloatColor emissive = material.getEmissiveColor();
	float shininess = material.getShininess();
#ifndef TARGET_OPENGLES
	// Material colors and properties
	glMaterialfv(GL_FRONT, GL_DIFFUSE, &diffuse.r);
	glMaterialfv(GL_FRONT, GL_SPECULAR, &specular.r);
	glMaterialfv(GL_FRONT, GL_AMBIENT, &ambient.r);
	glMaterialfv(GL_FRONT, GL_EMISSION, &emissive.r);
	glMaterialfv(GL_FRONT, GL_SHININESS, &shininess);

	glMaterialfv(GL_BACK, GL_DIFFUSE, &diffuse.r);
	glMaterialfv(GL_BACK, GL_SPECULAR, &specular.r);
	glMaterialfv(GL_BACK, GL_AMBIENT, &ambient.r);
	glMaterialfv(GL_BACK, GL_EMISSION, &emissive.r);
	glMaterialfv(GL_BACK, GL_SHININESS, &shininess);
#elif !defined(TARGET_PROGRAMMABLE_GL)

	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, &diffuse.r);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, &specular.r);
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, &ambient.r);
	glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, &emissive.r);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, &shininess);
#endif
}

//----------------------------------------------------------
void ofGLRenderer::unbind(const ofBaseMaterial & material){
	// Set default material colors and properties
	ofMaterial::Data defaultData;
#ifndef TARGET_OPENGLES
	glMaterialfv(GL_FRONT, GL_DIFFUSE, &defaultData.diffuse.r);
	glMaterialfv(GL_FRONT, GL_SPECULAR, &defaultData.specular.r);
	glMaterialfv(GL_FRONT, GL_AMBIENT, &defaultData.ambient.r);
	glMaterialfv(GL_FRONT, GL_EMISSION, &defaultData.emissive.r);
	glMaterialfv(GL_FRONT, GL_SHININESS, &defaultData.shininess);

	glMaterialfv(GL_BACK, GL_DIFFUSE, &defaultData.diffuse.r);
	glMaterialfv(GL_BACK, GL_SPECULAR, &defaultData.specular.r);
	glMaterialfv(GL_BACK, GL_AMBIENT, &defaultData.ambient.r);
	glMaterialfv(GL_BACK, GL_EMISSION, &defaultData.emissive.r);
	glMaterialfv(GL_BACK, GL_SHININESS, &defaultData.shininess);
#else
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, &defaultData.diffuse.r);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, &defaultData.specular.r);
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, &defaultData.ambient.r);
	glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, &defaultData.emissive.r);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, &defaultData.shininess);
#endif
}

//----------------------------------------------------------
void ofGLRenderer::bind(const ofTexture & texture, int location){
	//we could check if it has been allocated - but we don't do that in draw()
	if(texture.getAlphaMask()){
		setAlphaMaskTex(*texture.getAlphaMask());
	}
	enableTextureTarget(texture,location);


	if(ofGetUsingNormalizedTexCoords()) {
		matrixMode(OF_MATRIX_TEXTURE);
		pushMatrix();
		ofMatrix4x4 m;

#ifndef TARGET_OPENGLES
		if(texture.texData.textureTarget == GL_TEXTURE_RECTANGLE_ARB)
			m.makeScaleMatrix(texture.texData.width, texture.texData.height, 1.0f);
		else
#endif
			m.makeScaleMatrix(texture.texData.width / texture.texData.tex_w, texture.texData.height / texture.texData.tex_h, 1.0f);

		loadMatrix(m);
		matrixMode(OF_MATRIX_MODELVIEW);
	}
	if(texture.isUsingTextureMatrix()){
		matrixMode(OF_MATRIX_TEXTURE);
		if(!ofGetUsingNormalizedTexCoords()) pushMatrix();
		multMatrix(texture.getTextureMatrix());
		matrixMode(OF_MATRIX_MODELVIEW);
	}
}

//----------------------------------------------------------
void ofGLRenderer::unbind(const ofTexture & texture, int location){
	disableTextureTarget(texture.texData.textureTarget,location);
	if(texture.getAlphaMask()){
		disableAlphaMask();
	}

	if(texture.isUsingTextureMatrix() || ofGetUsingNormalizedTexCoords()) {
		matrixMode(OF_MATRIX_TEXTURE);
		popMatrix();
		matrixMode(OF_MATRIX_MODELVIEW);
	}
}

//----------------------------------------------------------
void ofGLRenderer::bind(const ofCamera & camera, const ofRectangle & _viewport){
	pushView();
	viewport(_viewport);
	setOrientation(matrixStack.getOrientation(),camera.isVFlipped());
	matrixMode(OF_MATRIX_PROJECTION);
	loadMatrix(camera.getProjectionMatrix(_viewport).getPtr());
	matrixMode(OF_MATRIX_MODELVIEW);
	loadViewMatrix(camera.getModelViewMatrix());
}

//----------------------------------------------------------
void ofGLRenderer::unbind(const ofCamera & camera){
	popView();
}

//----------------------------------------------------------
void ofGLRenderer::pushView() {
	getCurrentViewport();

	ofMatrix4x4 m;
	ofMatrixMode matrixMode = matrixStack.getCurrentMatrixMode();
	glGetFloatv(GL_PROJECTION_MATRIX,m.getPtr());
	matrixStack.matrixMode(OF_MATRIX_PROJECTION);
	matrixStack.loadMatrix(m.getPtr());
	glGetFloatv(GL_MODELVIEW_MATRIX,m.getPtr());
	matrixStack.matrixMode(OF_MATRIX_MODELVIEW);
	matrixStack.loadMatrix(m.getPtr());

	matrixStack.matrixMode(matrixMode);

	matrixStack.pushView();
}


//----------------------------------------------------------
void ofGLRenderer::popView() {
	matrixStack.popView();

	ofMatrix4x4 m;
	ofMatrixMode currentMode = matrixStack.getCurrentMatrixMode();

	matrixMode(OF_MATRIX_PROJECTION);
	loadMatrix(matrixStack.getProjectionMatrix());

	matrixMode(OF_MATRIX_MODELVIEW);
	loadMatrix(matrixStack.getModelViewMatrix());

	matrixMode(currentMode);

	viewport(matrixStack.getCurrentViewport());
}


//----------------------------------------------------------
void ofGLRenderer::viewport(ofRectangle viewport_){
	viewport(viewport_.x, viewport_.y, viewport_.width, viewport_.height, isVFlipped());
}

//----------------------------------------------------------
void ofGLRenderer::viewport(float x, float y, float width, float height, bool vflip) {
	matrixStack.viewport(x,y,width,height,vflip);
	ofRectangle nativeViewport = matrixStack.getNativeViewport();
	glViewport(nativeViewport.x,nativeViewport.y,nativeViewport.width,nativeViewport.height);
}

//----------------------------------------------------------
ofRectangle ofGLRenderer::getCurrentViewport() const{
	getNativeViewport();
	return matrixStack.getCurrentViewport();
}

//----------------------------------------------------------
ofRectangle ofGLRenderer::getNativeViewport() const{
	GLint viewport[4];					// Where The Viewport Values Will Be Stored
	glGetIntegerv(GL_VIEWPORT, viewport);

	ofGLRenderer * mutRenderer = const_cast<ofGLRenderer*>(this);
	ofRectangle nativeViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
	mutRenderer->matrixStack.nativeViewport(nativeViewport);
    return nativeViewport;
}

//----------------------------------------------------------
int ofGLRenderer::getViewportWidth() const{
	return getCurrentViewport().width;
}

//----------------------------------------------------------
int ofGLRenderer::getViewportHeight() const{
	return getCurrentViewport().height;
}

//----------------------------------------------------------
void ofGLRenderer::setCoordHandedness(ofHandednessType handedness) {

}

//----------------------------------------------------------
ofHandednessType ofGLRenderer::getCoordHandedness() const{
	return matrixStack.getHandedness();
}

//----------------------------------------------------------
void ofGLRenderer::setOrientation(ofOrientation orientation, bool vFlip){
	matrixStack.setOrientation(orientation,vFlip);
}

//----------------------------------------------------------
bool ofGLRenderer::isVFlipped() const{
	return matrixStack.isVFlipped();
}

//----------------------------------------------------------
bool ofGLRenderer::texturesNeedVFlip() const{
	return matrixStack.customMatrixNeedsFlip();
}

//----------------------------------------------------------
void ofGLRenderer::setupScreenPerspective(float width, float height, float fov, float nearDist, float farDist) {
	float viewW, viewH;
	if(width<0 || height<0){
		ofRectangle currentViewport = getCurrentViewport();

		viewW = currentViewport.width;
		viewH = currentViewport.height;
	}else{
		viewW = width;
		viewH = height;
	}

	float eyeX = viewW / 2;
	float eyeY = viewH / 2;
	float halfFov = PI * fov / 360;
	float theTan = tanf(halfFov);
	float dist = eyeY / theTan;
	float aspect = (float) viewW / viewH;

	if(nearDist == 0) nearDist = dist / 10.0f;
	if(farDist == 0) farDist = dist * 10.0f;


	matrixMode(OF_MATRIX_PROJECTION);
	ofMatrix4x4 persp;
	persp.makePerspectiveMatrix(fov, aspect, nearDist, farDist);
	loadMatrix( persp );

	matrixMode(OF_MATRIX_MODELVIEW);
	ofMatrix4x4 lookAt;
	lookAt.makeLookAtViewMatrix( ofVec3f(eyeX, eyeY, dist),  ofVec3f(eyeX, eyeY, 0),  ofVec3f(0, 1, 0) );
	loadViewMatrix(lookAt);
}

//----------------------------------------------------------
void ofGLRenderer::setupScreenOrtho(float width, float height, float nearDist, float farDist) {
	float viewW, viewH;
	if(width<0 || height<0){
		ofRectangle currentViewport = getCurrentViewport();

		viewW = currentViewport.width;
		viewH = currentViewport.height;
	}else{
		viewW = width;
		viewH = height;
	}

	ofMatrix4x4 ortho;

	ortho = ofMatrix4x4::newOrthoMatrix(0, viewW, 0, viewH, nearDist, farDist);

	matrixMode(OF_MATRIX_PROJECTION);
	loadMatrix(ortho); // make ortho our new projection matrix.

	matrixMode(OF_MATRIX_MODELVIEW);
	loadViewMatrix(ofMatrix4x4::newIdentityMatrix());

}

//----------------------------------------------------------
//Resets openGL parameters back to OF defaults
void ofGLRenderer::setupGraphicDefaults(){
	glEnableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	setStyle(ofStyle());
	path.setMode(ofPath::POLYLINES);
	path.setUseShapeColor(false);
}

//----------------------------------------------------------
void ofGLRenderer::setupScreen(){
	setupScreenPerspective();	// assume defaults
}

//----------------------------------------------------------
void ofGLRenderer::setCircleResolution(int res){
	if((int)circlePolyline.size()!=res+1){
		circlePolyline.clear();
		circlePolyline.arc(0,0,0,1,1,0,360,res);
		circlePoints.resize(circlePolyline.size());
		path.setCircleResolution(res);
	}
}

void ofGLRenderer::setPolyMode(ofPolyWindingMode mode){
	currentStyle.polyMode = mode;
	path.setPolyWindingMode(mode);
}

//our openGL wrappers
//----------------------------------------------------------
void ofGLRenderer::pushMatrix(){
	glPushMatrix();
}

//----------------------------------------------------------
void ofGLRenderer::popMatrix(){
	glPopMatrix();
}

//----------------------------------------------------------
void ofGLRenderer::translate(const ofPoint& p){
	glTranslatef(p.x, p.y, p.z);
}

//----------------------------------------------------------
void ofGLRenderer::translate(float x, float y, float z){
	glTranslatef(x, y, z);
}

//----------------------------------------------------------
void ofGLRenderer::scale(float xAmnt, float yAmnt, float zAmnt){
	glScalef(xAmnt, yAmnt, zAmnt);
}

//----------------------------------------------------------
void ofGLRenderer::rotate(float degrees, float vecX, float vecY, float vecZ){
	glRotatef(degrees, vecX, vecY, vecZ);
}

//----------------------------------------------------------
void ofGLRenderer::rotateX(float degrees){
	glRotatef(degrees, 1, 0, 0);
}

//----------------------------------------------------------
void ofGLRenderer::rotateY(float degrees){
	glRotatef(degrees, 0, 1, 0);
}

//----------------------------------------------------------
void ofGLRenderer::rotateZ(float degrees){
	glRotatef(degrees, 0, 0, 1);
}

//same as ofRotateZ
//----------------------------------------------------------
void ofGLRenderer::rotate(float degrees){
	glRotatef(degrees, 0, 0, 1);
}

//----------------------------------------------------------
void ofGLRenderer::matrixMode(ofMatrixMode mode){
	glMatrixMode(GL_MODELVIEW+mode);
	matrixStack.matrixMode(mode);
}

//----------------------------------------------------------
void ofGLRenderer::loadIdentityMatrix (void){
	loadMatrix(ofMatrix4x4::newIdentityMatrix());
}

//----------------------------------------------------------
void ofGLRenderer::loadMatrix (const ofMatrix4x4 & m){
	loadMatrix( m.getPtr() );
}

//----------------------------------------------------------
void ofGLRenderer::loadMatrix (const float *m){
	if(matrixStack.getCurrentMatrixMode()==OF_MATRIX_PROJECTION){
		matrixStack.loadMatrix(m);
		glLoadMatrixf(matrixStack.getProjectionMatrix().getPtr());
	}else{
		glLoadMatrixf(m);
	}
}

//----------------------------------------------------------

/** @brief	Queries the current OpenGL matrix state
 *  @detail Returns the specified matrix as held by the renderer's current matrix stack.
 *
 *			You can query one of the following:
 *
 *			[OF_MATRIX_MODELVIEW | OF_MATRIX_PROJECTION | OF_MATRIX_TEXTURE]
 *
 *			Each query will return the state of the matrix
 *			as it was uploaded to the shader currently bound.
 *
 *	@param	matrixMode_  Which matrix mode to query
 */
ofMatrix4x4 ofGLRenderer::getCurrentMatrix(ofMatrixMode matrixMode_) const {
	ofMatrix4x4 mat;
	switch (matrixMode_) {
		case OF_MATRIX_MODELVIEW:
			glGetFloatv(GL_MODELVIEW_MATRIX, mat.getPtr());
			break;
		case OF_MATRIX_PROJECTION:
			glGetFloatv(GL_PROJECTION_MATRIX, mat.getPtr());
			break;
		case OF_MATRIX_TEXTURE:
			glGetFloatv(GL_TEXTURE_MATRIX, mat.getPtr());
			break;
		default:
			ofLogWarning() << "Invalid getCurrentMatrix query";
			mat = ofMatrix4x4();
			break;
	}
	return mat;
}

//----------------------------------------------------------
ofMatrix4x4 ofGLRenderer::getCurrentOrientationMatrix() const {
	return matrixStack.getOrientationMatrix();
}

//----------------------------------------------------------
void ofGLRenderer::multMatrix (const ofMatrix4x4 & m){
	multMatrix( m.getPtr() );
}

//----------------------------------------------------------
void ofGLRenderer::multMatrix (const float *m){
	if(matrixStack.getCurrentMatrixMode()==OF_MATRIX_PROJECTION){
		ofMatrix4x4 current;
		glGetFloatv(GL_PROJECTION_MATRIX,current.getPtr());
		if(matrixStack.customMatrixNeedsFlip()){
			current.scale(1,-1,1);
		}
		matrixStack.loadMatrix(current.getPtr());
		matrixStack.multMatrix(m);
		glLoadMatrixf(matrixStack.getProjectionMatrix().getPtr());
	}else{
		glMultMatrixf(m);
	}
}

//----------------------------------------------------------
void ofGLRenderer::loadViewMatrix(const ofMatrix4x4 & m){
	int matrixMode;
	glGetIntegerv(GL_MATRIX_MODE,&matrixMode);
	matrixStack.loadViewMatrix(m);
	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf(m.getPtr());
	glMatrixMode(matrixMode);

	if(lightingEnabled){
		for(size_t i=0;i<ofLightsData().size();i++){
			shared_ptr<ofLight::Data> lightData = ofLightsData()[i].lock();
			if(lightData && lightData->isEnabled){
				glLightfv(GL_LIGHT0 + lightData->glIndex, GL_POSITION, &lightData->position.x);
			}
		}
	}
}

//----------------------------------------------------------
void ofGLRenderer::multViewMatrix(const ofMatrix4x4 & m){
	ofLogError() << "mutlViewMatrix not implemented on fixed GL renderer";
}

//----------------------------------------------------------
ofMatrix4x4 ofGLRenderer::getCurrentViewMatrix() const{
	return matrixStack.getViewMatrix();
}

//----------------------------------------------------------
ofMatrix4x4 ofGLRenderer::getCurrentNormalMatrix() const{
	return ofMatrix4x4::getTransposedOf(getCurrentMatrix(OF_MATRIX_MODELVIEW).getInverse());
}

//----------------------------------------------------------
void ofGLRenderer::setColor(const ofColor & color){
	setColor(color.r,color.g,color.b,color.a);
}

//----------------------------------------------------------
void ofGLRenderer::setColor(const ofColor & color, int _a){
	setColor(color.r,color.g,color.b,_a);
}

//----------------------------------------------------------
void ofGLRenderer::setColor(int r, int g, int b){
	currentStyle.color.set(r,g,b);
	glColor4f(r/255.f,g/255.f,b/255.f,1.f);
}


//----------------------------------------------------------
void ofGLRenderer::setColor(int r, int g, int b, int a){
	currentStyle.color.set(r,g,b,a);
	glColor4f(r/255.f,g/255.f,b/255.f,a/255.f);
}

//----------------------------------------------------------
void ofGLRenderer::setColor(int gray){
	setColor(gray, gray, gray);
}

//----------------------------------------------------------
void ofGLRenderer::setHexColor(int hexColor){
	int r = (hexColor >> 16) & 0xff;
	int g = (hexColor >> 8) & 0xff;
	int b = (hexColor >> 0) & 0xff;
	setColor(r,g,b);
}

//----------------------------------------------------------
void ofGLRenderer::clear(){
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

//----------------------------------------------------------
void ofGLRenderer::clear(float r, float g, float b, float a) {
	glClearColor(r / 255., g / 255., b / 255., a / 255.);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

//----------------------------------------------------------
void ofGLRenderer::clear(float brightness, float a) {
	clear(brightness, brightness, brightness, a);
}

//----------------------------------------------------------
void ofGLRenderer::clearAlpha() {
	glColorMask(0, 0, 0, 1);
	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT);
	glColorMask(1, 1, 1, 1);
}

//----------------------------------------------------------
void ofGLRenderer::setBackgroundAuto(bool bAuto){
	bBackgroundAuto = bAuto;
}

//----------------------------------------------------------
bool ofGLRenderer::getBackgroundAuto(){
	return bBackgroundAuto;
}

//----------------------------------------------------------
ofColor ofGLRenderer::getBackgroundColor(){
	return currentStyle.bgColor;
}

//----------------------------------------------------------
void ofGLRenderer::setBackgroundColor(const ofColor & color){
	currentStyle.bgColor = color;
 	glClearColor(currentStyle.bgColor[0]/255.,currentStyle.bgColor[1]/255.,currentStyle.bgColor[2]/255., currentStyle.bgColor[3]/255.);
}

//----------------------------------------------------------
void ofGLRenderer::background(const ofColor & c){
	setBackgroundColor(c);
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

//----------------------------------------------------------
void ofGLRenderer::background(float brightness) {
	background(brightness);
}

//----------------------------------------------------------
void ofGLRenderer::background(int hexColor, float _a){
	background ( (hexColor >> 16) & 0xff, (hexColor >> 8) & 0xff, (hexColor >> 0) & 0xff, _a);
}

//----------------------------------------------------------
void ofGLRenderer::background(int r, int g, int b, int a){
	background(ofColor(r,g,b,a));
}

//----------------------------------------------------------
void ofGLRenderer::setFillMode(ofFillFlag fill){
	currentStyle.bFill = (fill==OF_FILLED);
	if(currentStyle.bFill){
		path.setFilled(true);
		path.setStrokeWidth(0);
		#ifndef TARGET_OPENGLES
			// GLES does not support glPolygonMode
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		#endif
	}else{
		path.setFilled(false);
		path.setStrokeWidth(currentStyle.lineWidth);
		#ifndef TARGET_OPENGLES
			// GLES does not support glPolygonMode
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		#endif
	}
}

//----------------------------------------------------------
ofFillFlag ofGLRenderer::getFillMode(){
	if(currentStyle.bFill){
		return OF_FILLED;
	}else{
		return OF_OUTLINE;
	}
}

//----------------------------------------------------------
void ofGLRenderer::setRectMode(ofRectMode mode){
	currentStyle.rectMode = mode;
}

//----------------------------------------------------------
ofRectMode ofGLRenderer::getRectMode(){
	return currentStyle.rectMode;
}

//----------------------------------------------------------
void ofGLRenderer::setLineWidth(float lineWidth){
	currentStyle.lineWidth = lineWidth;
	if(!currentStyle.bFill){
		path.setStrokeWidth(lineWidth);
	}
	glLineWidth(lineWidth);
}

//----------------------------------------------------------
void ofGLRenderer::setDepthTest(bool depthTest){
	if(depthTest) {
		glEnable(GL_DEPTH_TEST);
	} else {
		glDisable(GL_DEPTH_TEST);
	}
}

//----------------------------------------------------------
void ofGLRenderer::setLineSmoothing(bool smooth){
	currentStyle.smoothing = smooth;
}


//----------------------------------------------------------
void ofGLRenderer::startSmoothing(){
	#ifndef TARGET_OPENGLES
		glPushAttrib(GL_COLOR_BUFFER_BIT | GL_ENABLE_BIT);
	#endif

	glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
	glEnable(GL_LINE_SMOOTH);

	//why do we need this?
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}


//----------------------------------------------------------
void ofGLRenderer::endSmoothing(){
	#ifndef TARGET_OPENGLES
		glPopAttrib();
	#endif
}

//----------------------------------------------------------
void ofGLRenderer::setBlendMode(ofBlendMode blendMode){
	switch (blendMode){
		case OF_BLENDMODE_DISABLED:
			glDisable(GL_BLEND);
			break;

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
			ofLogWarning("ofGLRenderer") << "OF_BLENDMODE_SUBTRACT not currently supported on OpenGL ES";
		#endif
			glBlendFunc(GL_SRC_ALPHA, GL_ONE);
			break;
		}

		default:
			break;
	}
	currentStyle.blendingMode = blendMode;
}

void ofGLRenderer::setBitmapTextMode(ofDrawBitmapMode mode){
	currentStyle.drawBitmapMode = mode;
}

ofStyle ofGLRenderer::getStyle() const{
	return currentStyle;
}

void ofGLRenderer::pushStyle(){
	styleHistory.push_back(currentStyle);
	//if we are over the max number of styles we have set, then delete the oldest styles.
	if( styleHistory.size() > OF_MAX_STYLE_HISTORY ){
		styleHistory.pop_front();
		//should we warn here?
		ofLogWarning("ofGraphics") << "ofPushStyle(): maximum number of style pushes << " << OF_MAX_STYLE_HISTORY << " reached, did you forget to pop somewhere?";
	}
}

void ofGLRenderer::popStyle(){
	if( styleHistory.size() ){
		setStyle(styleHistory.front());
		styleHistory.pop_back();
	}
}

void ofGLRenderer::setStyle(const ofStyle & style){
	//color
	setColor((int)style.color.r, (int)style.color.g, (int)style.color.b, (int)style.color.a);

	//bg color
	setBackgroundColor(style.bgColor);

	//circle resolution - don't worry it only recalculates the display list if the res has changed
	setCircleResolution(style.circleResolution);

	setCurveResolution(style.curveResolution);

	//line width - finally!
	setLineWidth(style.lineWidth);

	//ofSetDepthTest(style.depthTest); removed since it'll break old projects setting depth test through glEnable

	//rect mode: corner/center
	setRectMode(style.rectMode);

	//poly mode: winding type
	setPolyMode(style.polyMode);

	//fill
	if(style.bFill ){
		setFillMode(OF_FILLED);
	}else{
		setFillMode(OF_OUTLINE);
	}

	//smoothing
	/*if(style.smoothing ){
		enableSmoothing();
	}else{
		disableSmoothing();
	}*/

	//blending
	setBlendMode(style.blendingMode);

	currentStyle = style;
}

void ofGLRenderer::setCurveResolution(int resolution){
	currentStyle.curveResolution = resolution;
	path.setCurveResolution(resolution);
}

//----------------------------------------------------------
void ofGLRenderer::enablePointSprites(){

#ifdef TARGET_OPENGLES
	glEnable(GL_POINT_SPRITE_OES);
	glTexEnvi(GL_POINT_SPRITE_OES, GL_COORD_REPLACE_OES, GL_TRUE);
	// does look like this needs to be enabled in ES because
	// it is always eneabled...
	//glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);

#else
	glEnable(GL_POINT_SPRITE);
	glTexEnvi(GL_POINT_SPRITE, GL_COORD_REPLACE, GL_TRUE);
	glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
#endif

}

//----------------------------------------------------------
void ofGLRenderer::disablePointSprites(){

#ifdef TARGET_OPENGLES
	glDisable(GL_POINT_SPRITE_OES);
#else
	glDisable(GL_POINT_SPRITE);
#endif
}

//----------------------------------------------------------
void ofGLRenderer::enableAntiAliasing(){
	glEnable(GL_MULTISAMPLE);
}

//----------------------------------------------------------
void ofGLRenderer::disableAntiAliasing(){
	glDisable(GL_MULTISAMPLE);
}

//----------------------------------------------------------
void ofGLRenderer::drawLine(float x1, float y1, float z1, float x2, float y2, float z2) const{
	linePoints[0].set(x1,y1,z1);
	linePoints[1].set(x2,y2,z2);

	// use smoothness, if requested:
	if (currentStyle.smoothing) const_cast<ofGLRenderer*>(this)->startSmoothing();

	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, sizeof(ofVec3f), &linePoints[0].x);
	glDrawArrays(GL_LINES, 0, 2);

	// use smoothness, if requested:
	if (currentStyle.smoothing) const_cast<ofGLRenderer*>(this)->endSmoothing();

}

//----------------------------------------------------------
void ofGLRenderer::drawRectangle(float x, float y, float z,float w, float h) const{

	if (currentStyle.rectMode == OF_RECTMODE_CORNER){
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
	if (currentStyle.smoothing && !currentStyle.bFill) const_cast<ofGLRenderer*>(this)->startSmoothing();

	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, sizeof(ofVec3f), &rectPoints[0].x);
	glDrawArrays(currentStyle.bFill ? GL_TRIANGLE_FAN : GL_LINE_LOOP, 0, 4);

	// use smoothness, if requested:
	if (currentStyle.smoothing && !currentStyle.bFill) const_cast<ofGLRenderer*>(this)->endSmoothing();

}

//----------------------------------------------------------
void ofGLRenderer::drawTriangle(float x1, float y1, float z1, float x2, float y2, float z2, float x3, float y3, float z3) const{
	triPoints[0].set(x1,y1,z1);
	triPoints[1].set(x2,y2,z2);
	triPoints[2].set(x3,y3,z3);

	// use smoothness, if requested:
	if (currentStyle.smoothing && !currentStyle.bFill) const_cast<ofGLRenderer*>(this)->startSmoothing();

	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, sizeof(ofVec3f), &triPoints[0].x);
	glDrawArrays(currentStyle.bFill ? GL_TRIANGLE_FAN : GL_LINE_LOOP, 0, 3);

	// use smoothness, if requested:
	if (currentStyle.smoothing && !currentStyle.bFill) const_cast<ofGLRenderer*>(this)->endSmoothing();

}

//----------------------------------------------------------
void ofGLRenderer::drawCircle(float x, float y, float z,  float radius) const{
	const vector<ofPoint> & circleCache = circlePolyline.getVertices();
	for(int i=0;i<(int)circleCache.size();i++){
		circlePoints[i].set(radius*circleCache[i].x+x,radius*circleCache[i].y+y,z);
	}

	// use smoothness, if requested:
	if (currentStyle.smoothing && !currentStyle.bFill) const_cast<ofGLRenderer*>(this)->startSmoothing();

	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, sizeof(ofVec3f), &circlePoints[0].x);
	glDrawArrays(currentStyle.bFill ? GL_TRIANGLE_FAN : GL_LINE_STRIP, 0, circlePoints.size());

	// use smoothness, if requested:
	if (currentStyle.smoothing && !currentStyle.bFill) const_cast<ofGLRenderer*>(this)->endSmoothing();

}

//----------------------------------------------------------
void ofGLRenderer::drawEllipse(float x, float y, float z, float width, float height) const{
	float radiusX = width*0.5;
	float radiusY = height*0.5;
	const vector<ofPoint> & circleCache = circlePolyline.getVertices();
	for(int i=0;i<(int)circleCache.size();i++){
		circlePoints[i].set(radiusX*circlePolyline[i].x+x,radiusY*circlePolyline[i].y+y,z);
	}

	// use smoothness, if requested:
	if (currentStyle.smoothing && !currentStyle.bFill) const_cast<ofGLRenderer*>(this)->startSmoothing();

	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, sizeof(ofVec3f), &circlePoints[0].x);
	glDrawArrays(currentStyle.bFill ? GL_TRIANGLE_FAN : GL_LINE_STRIP, 0, circlePoints.size());

	// use smoothness, if requested:
	if (currentStyle.smoothing && !currentStyle.bFill) const_cast<ofGLRenderer*>(this)->endSmoothing();

}

//----------------------------------------------------------
void ofGLRenderer::drawString(string textString, float x, float y, float z) const{

	ofGLRenderer * mutThis = const_cast<ofGLRenderer*>(this);
	float sx = 0;
	float sy = 0;


	///////////////////////////
	// APPLY TRANSFORM / VIEW
	///////////////////////////
	//

	bool hasModelView = false;
	bool hasProjection = false;
	bool hasViewport = false;

	ofRectangle rViewport;

	switch (currentStyle.drawBitmapMode) {

		case OF_BITMAPMODE_SIMPLE:

			sx += x;
			sy += y;
			break;

		case OF_BITMAPMODE_SCREEN:

			hasViewport = true;
			mutThis->pushView();

			rViewport = matrixStack.getFullSurfaceViewport();
			mutThis->viewport(rViewport);

			mutThis->matrixMode(OF_MATRIX_PROJECTION);
			mutThis->loadIdentityMatrix();
			mutThis->matrixMode(OF_MATRIX_MODELVIEW);
			mutThis->loadIdentityMatrix();

			mutThis->translate(-1, 1, 0);
			mutThis->scale(2/rViewport.width, -2/rViewport.height, 1);

			mutThis->translate(x, y, 0);
			break;

		case OF_BITMAPMODE_VIEWPORT:

			rViewport = getCurrentViewport();

			hasProjection = true;
			mutThis->matrixMode(OF_MATRIX_PROJECTION);
			mutThis->pushMatrix();
			mutThis->loadIdentityMatrix();

			hasModelView = true;
			mutThis->matrixMode(OF_MATRIX_MODELVIEW);
			mutThis->pushMatrix();
			mutThis->loadIdentityMatrix();

			mutThis->translate(-1, 1, 0);
			mutThis->scale(2/rViewport.width, -2/rViewport.height, 1);

			mutThis->translate(x, y, 0);
			break;

		case OF_BITMAPMODE_MODEL:

			hasModelView = true;
			mutThis->matrixMode(OF_MATRIX_MODELVIEW);
			mutThis->pushMatrix();

			mutThis->translate(x, y, z);
			break;

		case OF_BITMAPMODE_MODEL_BILLBOARD:
		{
			//our aim here is to draw to screen
			//at the viewport position related
			//to the world position x,y,z

			// tig: we want to get the signed normalised screen coordinates (-1,+1) of our point (x,y,z)
			// that's projection * modelview * point in GLSL multiplication order
			// then doing the good old (v + 1.0) / 2. to get unsigned normalized screen (0,1) coordinates.
			// we then multiply x by width and y by height to get window coordinates.

			// previous implementations used gluProject, which made it incompatible with GLES (and the future)
			// https://developer.apple.com/library/mac/#documentation/Darwin/Reference/ManPages/man3/gluProject.3.html
			//
			// this could probably be backported to the GL2 Renderer =)

			rViewport = getCurrentViewport();

			ofMatrix4x4 modelview, projection;
			glGetFloatv(GL_MODELVIEW_MATRIX, modelview.getPtr());
			glGetFloatv(GL_PROJECTION_MATRIX, projection.getPtr());

			ofVec3f dScreen = ofVec3f(x,y,z) * modelview * projection * matrixStack.getOrientationMatrixInverse();
			dScreen += ofVec3f(1.0) ;
			dScreen *= 0.5;

			dScreen.x += rViewport.x;
			dScreen.x *= rViewport.width;

			dScreen.y += rViewport.y;
			dScreen.y *= rViewport.height;

			if (dScreen.z >= 1) return;


			hasProjection = true;
			mutThis->matrixMode(OF_MATRIX_PROJECTION);
			mutThis->pushMatrix();
			mutThis->loadIdentityMatrix();

			hasModelView = true;
			mutThis->matrixMode(OF_MATRIX_MODELVIEW);
			mutThis->pushMatrix();
			mutThis->loadIdentityMatrix();

			mutThis->translate(-1, -1, 0);

			mutThis->scale(2/rViewport.width, 2/rViewport.height, 1);

			mutThis->translate(dScreen.x, dScreen.y, 0);
		}
			break;

		default:
			break;
	}
	// remember the current blend mode so that we can restore it at the end of this method.
	GLint blend_src, blend_dst;
	glGetIntegerv( GL_BLEND_SRC, &blend_src );
	glGetIntegerv( GL_BLEND_DST, &blend_dst );

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#ifndef TARGET_OPENGLES
	// this temporarily enables alpha testing,
	// which discards pixels unless their alpha is 1.0f
	glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT);
	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0);
#endif

	ofMesh charMesh = bitmapFont.getMesh(textString,sx,sy,currentStyle.drawBitmapMode,isVFlipped());
	mutThis->bind(bitmapFont.getTexture(),0);
	draw(charMesh,OF_MESH_FILL,false,true,false);
	mutThis->unbind(bitmapFont.getTexture(),0);

#ifndef TARGET_OPENGLES
	glPopAttrib();
#endif
	// restore blendmode
	glBlendFunc(blend_src, blend_dst);

	if (hasModelView)
		mutThis->popMatrix();

	if (hasProjection)
	{
		mutThis->matrixMode(OF_MATRIX_PROJECTION);
		mutThis->popMatrix();
		mutThis->matrixMode(OF_MATRIX_MODELVIEW);
	}

	if (hasViewport)
		mutThis->popView();

}

//----------------------------------------------------------
void ofGLRenderer::drawString(const ofTrueTypeFont & font, string text, float x, float y) const{
	ofGLRenderer * mutThis = const_cast<ofGLRenderer*>(this);
	bool blendEnabled = glIsEnabled(GL_BLEND);
	GLint blend_src, blend_dst;
	glGetIntegerv( GL_BLEND_SRC, &blend_src );
	glGetIntegerv( GL_BLEND_DST, &blend_dst );

    glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	mutThis->bind(font.getFontTexture(),0);
	draw(font.getStringMesh(text,x,y,isVFlipped()),OF_MESH_FILL);
	mutThis->unbind(font.getFontTexture(),0);

	if(!blendEnabled){
		glDisable(GL_BLEND);
	}
	glBlendFunc(blend_src, blend_dst);
}

//----------------------------------------------------------
void ofGLRenderer::enableTextureTarget(const ofTexture & tex, int textureLocation){
	glActiveTexture(GL_TEXTURE0+textureLocation);
	glClientActiveTexture(GL_TEXTURE0+textureLocation);
	glEnable( tex.getTextureData().textureTarget);
	glBindTexture( tex.getTextureData().textureTarget, (GLuint)tex.getTextureData().textureID);
	textureLocationsEnabled.insert(textureLocation);
}

//----------------------------------------------------------
void ofGLRenderer::disableTextureTarget(int textureTarget, int textureLocation){
	glActiveTexture(GL_TEXTURE0+textureLocation);
	glBindTexture( textureTarget, 0);
	glDisable(textureTarget);
	glActiveTexture(GL_TEXTURE0);
	textureLocationsEnabled.erase(textureLocation);
}

void ofGLRenderer::setAlphaMaskTex(const ofTexture & tex){
	enableTextureTarget(tex, 1);
	alphaMaskTextureTarget = tex.getTextureData().textureTarget;
}

void ofGLRenderer::disableAlphaMask(){
	disableTextureTarget(alphaMaskTextureTarget,1);
}

//----------------------------------------------------------
void ofGLRenderer::enableLighting(){
	glEnable(GL_LIGHTING);
#ifndef TARGET_OPENGLES  //TODO: fix this
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
#endif
	glEnable(GL_COLOR_MATERIAL);

	// FIXME: we do this so the 3d ofDraw* functions work with lighting
	// but if someone enables it between ofEnableLighting it'll be disabled
	// on ofDisableLighting. by now it seems the best option to not loose
	// performance when drawing lots of primitives
	normalsEnabled = glIsEnabled( GL_NORMALIZE );
	glEnable(GL_NORMALIZE);
	lightingEnabled = true;
}

//----------------------------------------------------------
void ofGLRenderer::disableLighting(){
	glDisable(GL_LIGHTING);
	if(!normalsEnabled){
		glDisable(GL_NORMALIZE);
	}
	lightingEnabled = false;
}

//----------------------------------------------------------
void ofGLRenderer::enableSeparateSpecularLight(){
#ifndef TARGET_OPENGLES
	glLightModeli (GL_LIGHT_MODEL_COLOR_CONTROL,GL_SEPARATE_SPECULAR_COLOR);
#endif
}

//----------------------------------------------------------
void ofGLRenderer::disableSeparateSpecularLight(){
#ifndef TARGET_OPENGLES
	glLightModeli (GL_LIGHT_MODEL_COLOR_CONTROL,GL_SINGLE_COLOR);
#endif
}

//----------------------------------------------------------
bool ofGLRenderer::getLightingEnabled(){
	return glIsEnabled(GL_LIGHTING);
}

//----------------------------------------------------------
void ofGLRenderer::setSmoothLighting(bool b){
	if (b) glShadeModel(GL_SMOOTH);
	else glShadeModel(GL_FLAT);
}

//----------------------------------------------------------
void ofGLRenderer::setGlobalAmbientColor(const ofColor& c){
	GLfloat cc[] = {c.r/255.f, c.g/255.f, c.b/255.f, c.a/255.f};
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, cc);
}

//----------------------------------------------------------
void ofGLRenderer::enableLight(int lightIndex){
	enableLighting();
	glEnable(GL_LIGHT0 + lightIndex);
}

//----------------------------------------------------------
void ofGLRenderer::disableLight(int lightIndex){
	if(lightIndex!=-1) {
		glDisable(GL_LIGHT0 + lightIndex);
	}
}

//----------------------------------------------------------
void ofGLRenderer::setLightSpotlightCutOff(int lightIndex, float spotCutOff){
	glLightf(GL_LIGHT0 + lightIndex, GL_SPOT_CUTOFF, spotCutOff );
}

//----------------------------------------------------------
void ofGLRenderer::setLightSpotConcentration(int lightIndex, float exponent){
	glLightf(GL_LIGHT0 + lightIndex, GL_SPOT_EXPONENT, exponent);
}

//----------------------------------------------------------
void ofGLRenderer::setLightAttenuation(int lightIndex, float constant, float linear, float quadratic ){
    if(lightIndex==-1) return;
	glLightf(GL_LIGHT0 + lightIndex, GL_CONSTANT_ATTENUATION, constant);
	glLightf(GL_LIGHT0 + lightIndex, GL_LINEAR_ATTENUATION, linear);
	glLightf(GL_LIGHT0 + lightIndex, GL_QUADRATIC_ATTENUATION, quadratic);
}

//----------------------------------------------------------
void ofGLRenderer::setLightAmbientColor(int lightIndex, const ofFloatColor& c){
    if(lightIndex==-1) return;
	glLightfv(GL_LIGHT0 + lightIndex, GL_AMBIENT, &c.r);
}

//----------------------------------------------------------
void ofGLRenderer::setLightDiffuseColor(int lightIndex, const ofFloatColor& c){
    if(lightIndex==-1) return;
	glLightfv(GL_LIGHT0 + lightIndex, GL_DIFFUSE, &c.r);
}

//----------------------------------------------------------
void ofGLRenderer::setLightSpecularColor(int lightIndex, const ofFloatColor& c){
    if(lightIndex==-1) return;
	glLightfv(GL_LIGHT0 + lightIndex, GL_SPECULAR, &c.r);
}

//----------------------------------------------------------
void ofGLRenderer::setLightPosition(int lightIndex, const ofVec4f & position){
	if(lightIndex==-1) return;
	int matrixMode;
	glGetIntegerv(GL_MATRIX_MODE,&matrixMode);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadMatrixf(matrixStack.getViewMatrix().getPtr());
	glLightfv(GL_LIGHT0 + lightIndex, GL_POSITION, &position.x);
	glPopMatrix();
	glMatrixMode(matrixMode);
}

//----------------------------------------------------------
void ofGLRenderer::setLightSpotDirection(int lightIndex, const ofVec4f & direction){
	if(lightIndex==-1) return;
	glLightfv(GL_LIGHT0 + lightIndex, GL_SPOT_DIRECTION, &direction.x);
}

int ofGLRenderer::getGLVersionMajor(){
#ifdef TARGET_OPENGLES
	return 1;
#else
	return 2;
#endif
}

int ofGLRenderer::getGLVersionMinor(){
#ifdef TARGET_OPENGLES
	return 0;
#else
	return 1;
#endif
}


void ofGLRenderer::saveFullViewport(ofPixels & pixels){
	ofRectangle v = getCurrentViewport();
	saveScreen(v.x,v.y,v.width,v.height,pixels);
}

void ofGLRenderer::saveScreen(int x, int y, int w, int h, ofPixels & pixels){

    int sh = getViewportHeight();


	#ifndef TARGET_OPENGLES
	ofBufferObject buffer;
	pixels.allocate(w, h, OF_PIXELS_RGB);
	buffer.allocate(pixels.size(),GL_STATIC_READ);
	if(isVFlipped()){
		y = sh - y;
		y -= h; // top, bottom issues
	}

	buffer.bind(GL_PIXEL_PACK_BUFFER);
	glReadPixels(x, y, w, h, ofGetGlFormat(pixels), GL_UNSIGNED_BYTE, 0); // read the memory....
	buffer.unbind(GL_PIXEL_PACK_BUFFER);
	unsigned char * p = buffer.map<unsigned char>(GL_READ_ONLY);
	ofPixels src;
	src.setFromExternalPixels(p,w,h,OF_PIXELS_RGB);
	src.mirrorTo(pixels,true,false);
	buffer.unmap();

	#else

	int sw = getViewportWidth();
	int numPixels   = w*h;
	if( numPixels == 0 ){
		ofLogError("ofImage") << "grabScreen(): unable to grab screen, image width and/or height are 0: " << w << "x" << h;
		return;
	}
	pixels.allocate(w, h, OF_PIXELS_RGBA);

	switch(matrixStack.getOrientation()){
	case OF_ORIENTATION_DEFAULT:

		if(isVFlipped()){
			y = sh - y;   // screen is flipped vertically.
			y -= h;
		}

		glPixelStorei(GL_PACK_ALIGNMENT, 1);
		glReadPixels(x, y, w, h, GL_RGBA, GL_UNSIGNED_BYTE, pixels.getData());
		pixels.mirror(true,false);
		break;
	case OF_ORIENTATION_180:

		if(isVFlipped()){
			x = sw - x;   // screen is flipped horizontally.
			x -= w;
		}

		glPixelStorei(GL_PACK_ALIGNMENT, 1);
		glReadPixels(x, y, w, h, GL_RGBA, GL_UNSIGNED_BYTE, pixels.getData());
		pixels.mirror(false,true);
		break;
	case OF_ORIENTATION_90_RIGHT:
		swap(w,h);
		swap(x,y);
		if(!isVFlipped()){
			x = sw - x;   // screen is flipped horizontally.
			x -= w;

			y = sh - y;   // screen is flipped vertically.
			y -= h;
		}

		glPixelStorei(GL_PACK_ALIGNMENT, 1);
		glReadPixels(x, y, w, h, GL_RGBA, GL_UNSIGNED_BYTE, pixels.getData());
		pixels.mirror(true,true);
		break;
	case OF_ORIENTATION_90_LEFT:
		swap(w, h);
		swap(x, y);
		if(isVFlipped()){
			x = sw - x;   // screen is flipped horizontally.
			x -= w;

			y = sh - y;   // screen is flipped vertically.
			y -= h;
		}

		glPixelStorei(GL_PACK_ALIGNMENT, 1);
		glReadPixels(x, y, w, h, GL_RGBA, GL_UNSIGNED_BYTE, pixels.getData());
		pixels.mirror(true,true);
		break;
	}

	#endif
}

const of3dGraphics & ofGLRenderer::get3dGraphics() const{
	return graphics3d;
}

of3dGraphics & ofGLRenderer::get3dGraphics(){
	return graphics3d;
}
