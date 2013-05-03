#include "ofShader.h"
#include "ofUtils.h"
#include "ofFileUtils.h"
#include "ofGraphics.h"
#include "ofProgrammableGLRenderer.h"
#include <map>

static const string COLOR_ATTRIBUTE="color";
static const string POSITION_ATTRIBUTE="position";
static const string NORMAL_ATTRIBUTE="normal";
static const string TEXCOORD_ATTRIBUTE="texcoord";

static map<GLuint,int> & getShaderIds(){
	static map<GLuint,int> * ids = new map<GLuint,int>;
	return *ids;
}

static map<GLuint,int> & getProgramIds(){
	static map<GLuint,int> * ids = new map<GLuint,int>;
	return *ids;
}

//--------------------------------------------------------------
static void retainShader(GLuint id){
	if(id==0) return;
	if(getShaderIds().find(id)!=getShaderIds().end()){
		getShaderIds()[id]++;
	}else{
		getShaderIds()[id]=1;
	}
}

//--------------------------------------------------------------
static void releaseShader(GLuint program, GLuint id){
	if(getShaderIds().find(id)!=getShaderIds().end()){
		getShaderIds()[id]--;
		if(getShaderIds()[id]==0){
			glDetachShader(program, id);
			glDeleteShader(id);
			getShaderIds().erase(id);
		}
	}else{
		ofLog(OF_LOG_WARNING,"ofShader: releasing id not found, this shouldn't be happening releasing anyway");
		glDetachShader(program, id);
		glDeleteShader(id);
	}
}

//--------------------------------------------------------------
static void retainProgram(GLuint id){
	if(id==0) return;
	if(getProgramIds().find(id)!=getProgramIds().end()){
		getProgramIds()[id]++;
	}else{
		getProgramIds()[id]=1;
	}
}

//--------------------------------------------------------------
static void releaseProgram(GLuint id){
	if(getProgramIds().find(id)!=getProgramIds().end()){
		getProgramIds()[id]--;
		if(getProgramIds()[id]==0){
			glDeleteProgram(id);
			getProgramIds().erase(id);
		}
	}else{
		ofLog(OF_LOG_WARNING,"ofShader: releasing program not found, this shouldn't be happening releasing anyway");
		glDeleteProgram(id);
	}
}

//--------------------------------------------------------------
ofShader::ofShader() :
program(0),
bLoaded(false)
{
}

//--------------------------------------------------------------
ofShader::~ofShader() {
	unload();
}

//--------------------------------------------------------------
ofShader::ofShader(const ofShader & mom) :
program(mom.program),
bLoaded(mom.bLoaded),
shaders(mom.shaders){
	if(mom.bLoaded){
		retainProgram(program);
		for(map<GLenum, GLuint>::const_iterator it = shaders.begin(); it != shaders.end(); ++it){
			GLuint shader = it->second;
			retainShader(shader);
		}
	}
}

//--------------------------------------------------------------
ofShader & ofShader::operator=(const ofShader & mom){
    if(this == &mom) {
        return *this;
    }
	if(bLoaded){
		unload();
	}
	program = mom.program;
	bLoaded = mom.bLoaded;
	shaders = mom.shaders;
	if(mom.bLoaded){
		retainProgram(program);
		for(map<GLenum, GLuint>::const_iterator it = shaders.begin(); it != shaders.end(); ++it){
			GLuint shader = it->second;
			retainShader(shader);
		}
	}
	return *this;
}

//--------------------------------------------------------------
bool ofShader::load(string shaderName) {
	return load(shaderName + ".vert", shaderName + ".frag");
}

//--------------------------------------------------------------
bool ofShader::load(string vertName, string fragName, string geomName) {
	if(vertName.empty() == false) setupShaderFromFile(GL_VERTEX_SHADER, vertName);
	if(fragName.empty() == false) setupShaderFromFile(GL_FRAGMENT_SHADER, fragName);
#ifndef TARGET_OPENGLES
	if(geomName.empty() == false) setupShaderFromFile(GL_GEOMETRY_SHADER_EXT, geomName);
#endif
	if(ofGetProgrammableGLRenderer()){
		bindDefaults();
	}
	return linkProgram();
}

//--------------------------------------------------------------
bool ofShader::setupShaderFromFile(GLenum type, string filename) {
	ofBuffer buffer = ofBufferFromFile(filename);
	if(buffer.size()) {
		return setupShaderFromSource(type, buffer.getText());
	} else {
		ofLog(OF_LOG_ERROR, "Could not load shader of type " + nameForType(type) + " from file " + filename);
		return false;
	}
}

//--------------------------------------------------------------
bool ofShader::setupShaderFromSource(GLenum type, string source) {
    unload();
    
	// create program if it doesn't exist already
	checkAndCreateProgram();

	
	// create shader
	GLuint shader = glCreateShader(type);
	if(shader == 0) {
		ofLog(OF_LOG_ERROR, "Failed creating shader of type " + nameForType(type));
		return false;
	}
	
	// compile shader
	const char * sptr = source.c_str();
	int ssize = source.size();
	glShaderSource(shader, 1, &sptr, &ssize);
	glCompileShader(shader);
	
	// check compile status
	GLint status = GL_FALSE;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    GLuint err = glGetError();
    if (err != GL_NO_ERROR){
        ofLog( OF_LOG_ERROR, "OpenGL generated error " + ofToString(err) + " trying to get the compile status for " + nameForType(type) + " shader. Does your video card support this?" );
        return false;
    }
    
	if(status == GL_TRUE){
		ofLog(OF_LOG_VERBOSE, nameForType(type) + " shader compiled.");
		checkShaderInfoLog(shader, type, OF_LOG_WARNING);
	}
	
	else if (status == GL_FALSE) {
		ofLog(OF_LOG_ERROR, nameForType(type) + " shader failed to compile");
		checkShaderInfoLog(shader, type, OF_LOG_ERROR);
		return false;
	}
	
	shaders[type] = shader;
	retainShader(shader);

	return true;
}

//--------------------------------------------------------------
void ofShader::setGeometryInputType(GLenum type) {
#ifndef TARGET_OPENGLES
	checkAndCreateProgram();
	glProgramParameteriEXT(program, GL_GEOMETRY_INPUT_TYPE_EXT, type);
#endif
}

//--------------------------------------------------------------
void ofShader::setGeometryOutputType(GLenum type) {
#ifndef TARGET_OPENGLES
	checkAndCreateProgram();
	glProgramParameteriEXT(program, GL_GEOMETRY_OUTPUT_TYPE_EXT, type);
#endif
}

//--------------------------------------------------------------
void ofShader::setGeometryOutputCount(int count) {
#ifndef TARGET_OPENGLES
	checkAndCreateProgram();
	glProgramParameteriEXT(program, GL_GEOMETRY_VERTICES_OUT_EXT, count);
#endif
}

//--------------------------------------------------------------
int ofShader::getGeometryMaxOutputCount() {
#ifndef TARGET_OPENGLES
	int temp;
	glGetIntegerv(GL_MAX_GEOMETRY_OUTPUT_VERTICES_EXT, &temp);
	return temp;
#else
	return 0;
#endif
}

//--------------------------------------------------------------
bool ofShader::checkProgramLinkStatus(GLuint program) {
	GLint status;
	glGetProgramiv(program, GL_LINK_STATUS, &status);
    GLuint err = glGetError();
    if (err != GL_NO_ERROR){
        ofLog( OF_LOG_ERROR, "OpenGL generated error "+ofToString(err)+" trying to get the program link status. Does your video card support shader programs?" );
        return false;
    }
	if(status == GL_TRUE)
		ofLog(OF_LOG_VERBOSE, "Program linked.");
	else if (status == GL_FALSE) {
		ofLog(OF_LOG_ERROR, "Program failed to link.");
		checkProgramInfoLog(program);
		return false;
	}
	return true;
}

//--------------------------------------------------------------
void ofShader::checkShaderInfoLog(GLuint shader, GLenum type, ofLogLevel logLevel) {
	GLsizei infoLength;
	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLength);
	if (infoLength > 1) {
		GLchar* infoBuffer = new GLchar[infoLength];
		glGetShaderInfoLog(shader, infoLength, &infoLength, infoBuffer);
		ofLog(logLevel, nameForType(type) + " shader reports:\n" + infoBuffer);
		delete [] infoBuffer;
	}
}

//--------------------------------------------------------------
void ofShader::checkProgramInfoLog(GLuint program) {
	GLsizei infoLength;
	glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLength);
	if (infoLength > 1) {
		GLchar* infoBuffer = new GLchar[infoLength];
		glGetProgramInfoLog(program, infoLength, &infoLength, infoBuffer);
		string msg = "Shader program reports:\n";
		ofLog(OF_LOG_ERROR, msg + infoBuffer);
		delete [] infoBuffer;
	}
}



//--------------------------------------------------------------
void ofShader::checkAndCreateProgram() {
#ifndef TARGET_OPENGLES
	if(GL_ARB_shader_objects) {
#else
	if(true){
#endif
		if(program == 0) {
			ofLog(OF_LOG_VERBOSE, "Creating GLSL Program");
			program = glCreateProgram();
			retainProgram(program);
		}
	} else {
		ofLog(OF_LOG_ERROR, "Sorry, it looks like you can't run 'ARB_shader_objects'.\nPlease check the capabilites of your graphics card.\nhttp://www.ozone3d.net/gpu_caps_viewer/");
	}
}

//--------------------------------------------------------------
bool ofShader::linkProgram() {
		if(shaders.empty()) {
			ofLog(OF_LOG_ERROR, "Trying to link GLSL program, but no shaders created yet");
		} else {
			checkAndCreateProgram();

			for(map<GLenum, GLuint>::const_iterator it = shaders.begin(); it != shaders.end(); ++it){
				GLuint shader = it->second;
				if(shader) {
					ofLog(OF_LOG_VERBOSE, "Attaching shader of type " + nameForType(it->first));
					glAttachShader(program, shader);
				}
			}
			
			glLinkProgram(program);
            
            checkProgramLinkStatus(program);

            // bLoaded means we have loaded shaders onto the graphics card;
            // it doesn't necessarily mean that these shaders have compiled and linked successfully.
			bLoaded = true;
		}
		return bLoaded;
}

void ofShader::bindAttribute(GLuint location, const string & name){
	glBindAttribLocation(program,location,name.c_str());
}

//--------------------------------------------------------------
bool ofShader::bindDefaults(){
	if(shaders.empty()) {
		ofLog(OF_LOG_ERROR, "Trying to link GLSL program, but no shaders created yet");
		return false;
	} else {
		bindAttribute(ofShader::POSITION_ATTRIBUTE,::POSITION_ATTRIBUTE);
		bindAttribute(ofShader::COLOR_ATTRIBUTE,::COLOR_ATTRIBUTE);
		bindAttribute(ofShader::NORMAL_ATTRIBUTE,::NORMAL_ATTRIBUTE);
		bindAttribute(ofShader::TEXCOORD_ATTRIBUTE,::TEXCOORD_ATTRIBUTE);
		return true;
	}

}

//--------------------------------------------------------------
void ofShader::unload() {
	if(bLoaded) {
		for(map<GLenum, GLuint>::const_iterator it = shaders.begin(); it != shaders.end(); ++it) {
			GLuint shader = it->second;
			if(shader) {
				//ofLog(OF_LOG_VERBOSE, "Detaching and deleting shader of type " + nameForType(it->first));
				releaseShader(program,shader);
			}
		}

		if (program) {
			releaseProgram(program);
			program = 0;
		}

		shaders.clear();
	}
	bLoaded = false;
}

//--------------------------------------------------------------
bool ofShader::isLoaded(){
	return bLoaded;
}

//--------------------------------------------------------------
void ofShader::begin() {
	if (bLoaded){
		glUseProgram(program);
		if(ofGetProgrammableGLRenderer()){
			ofGetProgrammableGLRenderer()->beginCustomShader(*this);
		}
	}else{
		ofLogError() << "trying to begin unloaded shader";
	}
}

//--------------------------------------------------------------
void ofShader::end() {
	if (bLoaded){
		if(ofGetProgrammableGLRenderer()){
			ofGetProgrammableGLRenderer()->endCustomShader();
		}else{
			glUseProgram(0);
		}
	}
}

//--------------------------------------------------------------
void ofShader::setUniformTexture(const string & name, ofBaseHasTexture& img, int textureLocation) {
	setUniformTexture(name, img.getTextureReference(), textureLocation);
}

//--------------------------------------------------------------
void ofShader::setUniformTexture(const string & name, int textureTarget, GLint textureID, int textureLocation){
	if(bLoaded) {
		glActiveTexture(GL_TEXTURE0 + textureLocation);
		if (ofGLIsFixedPipeline()){
			glEnable(textureTarget);
			glBindTexture(textureTarget, textureID);
			glDisable(textureTarget);
		} else {
			glBindTexture(textureTarget, textureID);
		}
		setUniform1i(name, textureLocation);
		glActiveTexture(GL_TEXTURE0);
	}
}

//--------------------------------------------------------------
void ofShader::setUniformTexture(const string & name, ofTexture& tex, int textureLocation) {
	if(bLoaded) {
		ofTextureData texData = tex.getTextureData();
		glActiveTexture(GL_TEXTURE0 + textureLocation);
		if (ofGLIsFixedPipeline()){
			glEnable(texData.textureTarget);
			glBindTexture(texData.textureTarget, texData.textureID);
			glDisable(texData.textureTarget);
		} else {
			glBindTexture(texData.textureTarget, texData.textureID);
		}
		setUniform1i(name, textureLocation);
		glActiveTexture(GL_TEXTURE0);
	}
}

//--------------------------------------------------------------
void ofShader::setUniform1i(const string & name, int v1) {
	if(bLoaded) {
		int loc = getUniformLocation(name);
		if (loc != -1) glUniform1i(loc, v1);
	}
}

//--------------------------------------------------------------
void ofShader::setUniform2i(const string & name, int v1, int v2) {
	if(bLoaded) {
		int loc = getUniformLocation(name);
		if (loc != -1) glUniform2i(loc, v1, v2);
	}
}

//--------------------------------------------------------------
void ofShader::setUniform3i(const string & name, int v1, int v2, int v3) {
	if(bLoaded) {
		int loc = getUniformLocation(name);
		if (loc != -1) glUniform3i(loc, v1, v2, v3);
	}
}

//--------------------------------------------------------------
void ofShader::setUniform4i(const string & name, int v1, int v2, int v3, int v4) {
	if(bLoaded) {
		int loc = getUniformLocation(name);
		if (loc != -1) 	glUniform4i(loc, v1, v2, v3, v4);
	}
}

//--------------------------------------------------------------
void ofShader::setUniform1f(const string & name, float v1) {
	if(bLoaded) {
		int loc = getUniformLocation(name);
		if (loc != -1) glUniform1f(loc, v1);
	}
}

//--------------------------------------------------------------
void ofShader::setUniform2f(const string & name, float v1, float v2) {
	if(bLoaded) {
		int loc = getUniformLocation(name);
		if (loc != -1) glUniform2f(loc, v1, v2);
	}
}

//--------------------------------------------------------------
void ofShader::setUniform3f(const string & name, float v1, float v2, float v3) {
	if(bLoaded) {
		int loc = getUniformLocation(name);
		if (loc != -1) glUniform3f(loc, v1, v2, v3);
	}
}

//--------------------------------------------------------------
void ofShader::setUniform4f(const string & name, float v1, float v2, float v3, float v4) {
	if(bLoaded) {
		int loc = getUniformLocation(name);
		if (loc != -1) glUniform4f(loc, v1, v2, v3, v4);
	}
}

//--------------------------------------------------------------
void ofShader::setUniform1iv(const string & name, int* v, int count) {
	if(bLoaded) {
		int loc = getUniformLocation(name);
		if (loc != -1) glUniform1iv(loc, count, v);
	}
}

//--------------------------------------------------------------
void ofShader::setUniform2iv(const string & name, int* v, int count) {
	if(bLoaded) {
		int loc = getUniformLocation(name);
		if (loc != -1) glUniform2iv(loc, count, v);
	}
}

//--------------------------------------------------------------
void ofShader::setUniform3iv(const string & name, int* v, int count) {
	if(bLoaded) {
		int loc = getUniformLocation(name);
		if (loc != -1) glUniform3iv(loc, count, v);
	}
}

//--------------------------------------------------------------
void ofShader::setUniform4iv(const string & name, int* v, int count) {
	if(bLoaded) {
		int loc = getUniformLocation(name);
		if (loc != -1) glUniform4iv(loc, count, v);
	}
}

//--------------------------------------------------------------
void ofShader::setUniform1fv(const string & name, float* v, int count) {
	if(bLoaded) {
		int loc = getUniformLocation(name);
		if (loc != -1) glUniform1fv(loc, count, v);
	}
}

//--------------------------------------------------------------
void ofShader::setUniform2fv(const string & name, float* v, int count) {
	if(bLoaded) {
		int loc = getUniformLocation(name);
		if (loc != -1) glUniform2fv(loc, count, v);
	}
}

//--------------------------------------------------------------
void ofShader::setUniform3fv(const string & name, float* v, int count) {
	if(bLoaded) {
		int loc = getUniformLocation(name);
		if (loc != -1) glUniform3fv(loc, count, v);
	}
}

//--------------------------------------------------------------
void ofShader::setUniform4fv(const string & name, float* v, int count) {
	if(bLoaded) {
		int loc = getUniformLocation(name);
		if (loc != -1) glUniform4fv(loc, count, v);
	}
}

//--------------------------------------------------------------
void ofShader::setUniformMatrix4f(const string & name, const ofMatrix4x4 & m) {
	if(bLoaded) {
		int loc = getUniformLocation(name);
		if (loc != -1) glUniformMatrix4fv(loc, 1, GL_FALSE, m.getPtr());
	}
}

#ifndef TARGET_OPENGLES
//--------------------------------------------------------------
void ofShader::setAttribute1s(GLint location, short v1) {
	if(bLoaded)
		glVertexAttrib1s(location, v1);
}

//--------------------------------------------------------------
void ofShader::setAttribute2s(GLint location, short v1, short v2) {
	if(bLoaded)
		glVertexAttrib2s(location, v1, v2);
}

//--------------------------------------------------------------
void ofShader::setAttribute3s(GLint location, short v1, short v2, short v3) {
	if(bLoaded)
		glVertexAttrib3s(location, v1, v2, v3);
}

//--------------------------------------------------------------
void ofShader::setAttribute4s(GLint location, short v1, short v2, short v3, short v4) {
	if(bLoaded)
		glVertexAttrib4s(location, v1, v2, v3, v4);
}
#endif

//--------------------------------------------------------------
void ofShader::setAttribute1f(GLint location, float v1) {
	if(bLoaded)
		glVertexAttrib1f(location, v1);
}

//--------------------------------------------------------------
void ofShader::setAttribute2f(GLint location, float v1, float v2) {
	if(bLoaded)
		glVertexAttrib2f(location, v1, v2);
}

//--------------------------------------------------------------
void ofShader::setAttribute3f(GLint location, float v1, float v2, float v3) {
	if(bLoaded)
		glVertexAttrib3f(location, v1, v2, v3);
}

//--------------------------------------------------------------
void ofShader::setAttribute4f(GLint location, float v1, float v2, float v3, float v4) {
	if(bLoaded)
		glVertexAttrib4f(location, v1, v2, v3, v4);
}

void ofShader::setAttribute1fv(const string & name, float* v, GLsizei stride){
	if(bLoaded){
		GLint location = getAttributeLocation(name);
		if (location != -1) {
			glVertexAttribPointer(location, 1, GL_FLOAT, GL_FALSE, stride, v);
			glEnableVertexAttribArray(location);
		}
	}
}

void ofShader::setAttribute2fv(const string & name, float* v, GLsizei stride){
	if(bLoaded){
		GLint location = getAttributeLocation(name);
		if (location != -1) {
			glVertexAttribPointer(location, 2, GL_FLOAT, GL_FALSE, stride, v);
			glEnableVertexAttribArray(location);
		}
	}

}

void ofShader::setAttribute3fv(const string & name, float* v, GLsizei stride){
	if(bLoaded){
		GLint location = getAttributeLocation(name);
		if (location != -1) {
			glVertexAttribPointer(location, 3, GL_FLOAT, GL_FALSE, stride, v);
			glEnableVertexAttribArray(location);
		}
	}
}

void ofShader::setAttribute4fv(const string & name, float* v, GLsizei stride){
	if(bLoaded){
		GLint location = getAttributeLocation(name);
		if (location != -1) {
			glVertexAttribPointer(location, 4, GL_FLOAT, GL_FALSE, stride, v);
			glEnableVertexAttribArray(location);
		}
	}
}

#ifndef TARGET_OPENGLES
//--------------------------------------------------------------
void ofShader::setAttribute1d(GLint location, double v1) {
	if(bLoaded)
		glVertexAttrib1d(location, v1);
}

//--------------------------------------------------------------
void ofShader::setAttribute2d(GLint location, double v1, double v2) {
	if(bLoaded)
		glVertexAttrib2d(location, v1, v2);
}

//--------------------------------------------------------------
void ofShader::setAttribute3d(GLint location, double v1, double v2, double v3) {
	if(bLoaded)
		glVertexAttrib3d(location, v1, v2, v3);
}

//--------------------------------------------------------------
void ofShader::setAttribute4d(GLint location, double v1, double v2, double v3, double v4) {
	if(bLoaded)
		glVertexAttrib4d(location, v1, v2, v3, v4);
}
#endif

//--------------------------------------------------------------
GLint ofShader::getAttributeLocation(const string & name) {
	return glGetAttribLocation(program, name.c_str());
}

//--------------------------------------------------------------
GLint ofShader::getUniformLocation(const string & name) {
	return glGetUniformLocation(program, name.c_str());
}

//--------------------------------------------------------------
void ofShader::printActiveUniforms() {
	GLint numUniforms = 0;
	glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &numUniforms);
	cout << numUniforms << " uniforms:" << endl;
	
	GLint uniformMaxLength = 0;
	glGetProgramiv(program, GL_ACTIVE_UNIFORM_MAX_LENGTH, &uniformMaxLength);
	
	GLint count = -1;
	GLenum type = 0;
	GLchar* uniformName = new GLchar[uniformMaxLength];
	for(GLint i = 0; i < numUniforms; i++) {
		GLsizei length;
		glGetActiveUniform(program, i, uniformMaxLength, &length, &count, &type, uniformName);
		cout << " [" << i << "] ";
		for(int j = 0; j < length; j++)
			cout << uniformName[j];
		cout << " @ index " << getUniformLocation(uniformName) << endl;
	}
	delete [] uniformName;
}

//--------------------------------------------------------------
void ofShader::printActiveAttributes() {
	GLint numAttributes = 0;
	glGetProgramiv(program, GL_ACTIVE_ATTRIBUTES, &numAttributes);
	cout << numAttributes << " attributes:" << endl;
	
	GLint attributeMaxLength = 0;
	glGetProgramiv(program, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &attributeMaxLength);
	
	GLint count = -1;
	GLenum type = 0;
	GLchar* attributeName = new GLchar[attributeMaxLength];
	for(GLint i = 0; i < numAttributes; i++) {
		GLsizei length;
		glGetActiveAttrib(program, i, attributeMaxLength, &length, &count, &type, attributeName);
		cout << " [" << i << "] ";
		for(int j = 0; j < length; j++)
			cout <<attributeName[j];
		cout << " @ index " << getAttributeLocation(attributeName) << endl;
	}
	delete [] attributeName;
}

//--------------------------------------------------------------
GLuint& ofShader::getProgram() {
	return program;
}

//--------------------------------------------------------------
GLuint& ofShader::getShader(GLenum type) {
	return shaders[type];
}

//--------------------------------------------------------------
bool ofShader::operator==(const ofShader & other){
	return other.program==program;
}

//--------------------------------------------------------------
bool ofShader::operator!=(const ofShader & other){
	return other.program!=program;
}

//--------------------------------------------------------------
string ofShader::nameForType(GLenum type) {
	switch(type) {
		case GL_VERTEX_SHADER: return "GL_VERTEX_SHADER";
		case GL_FRAGMENT_SHADER: return "GL_FRAGMENT_SHADER";
		#ifndef TARGET_OPENGLES
		case GL_GEOMETRY_SHADER_EXT: return "GL_GEOMETRY_SHADER_EXT";
		#endif
		default: return "UNKNOWN SHADER TYPE";
	}
}


#ifdef TARGET_OPENGLES
string defaultVertexShader =
		"attribute vec4 position;\
		attribute vec4 color;\
		attribute vec4 normal;\
		attribute vec2 texcoord;\
		\
		uniform mat4 modelViewMatrix;\
		uniform mat4 projectionMatrix;\
		uniform mat4 textureMatrix;\
		uniform mat4 modelViewProjectionMatrix;\
		\
		varying vec4 colorVarying;\
		varying vec2 texCoordVarying;\
        \
		void main(){\
			gl_Position = modelViewProjectionMatrix * position;\
			colorVarying = color;\
			texCoordVarying = (textureMatrix*vec4(texcoord.x,texcoord.y,0,1)).xy;\
		}";

string defaultFragmentShaderTexColor =
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
		    gl_FragColor = texture2D(src_tex_unit0, texCoordVarying)*colorVarying;\
        }";

string defaultFragmentShaderTexNoColor =
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
		    gl_FragColor = texture2D(src_tex_unit0, texCoordVarying)*color;\
        }";

string defaultFragmentShaderNoTexColor =
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
		    gl_FragColor = colorVarying;\
        }";

string defaultFragmentShaderNoTexNoColor =
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
		    gl_FragColor = color;\
        }";

// tig: todo: implement shaders for bitmapstringdraw on GLES2...
// GLSL_ES shader written against spec:
// http://www.khronos.org/registry/gles/specs/2.0/GLSL_ES_Specification_1.0.17.pdf

string bitmapStringVertexShader		= "\n\
#ifdef GL_ES\n\
// define default precision for float, vec, mat.\n\
precision highp float;\n\
#endif\n\
\n\
uniform mat4 projectionMatrix;\n\
uniform mat4 modelViewMatrix;\n\
uniform mat4 textureMatrix;\
uniform mat4 modelViewProjectionMatrix;\n\
\n\
attribute vec4  position;\n\
attribute vec2  texcoord;\n\
\n\
varying vec2 texCoordVarying;\n\
\n\
void main()\n\
{\n\
	texCoordVarying = (textureMatrix*vec4(texcoord.x,texcoord.y,0,1)).xy;\n\
	gl_Position = modelViewProjectionMatrix * position;\n\
}";
string bitmapStringFragmentShader	= "\n\
#ifdef GL_ES\n\
// define default precision for float, vec, mat.\n\
precision highp float;\n\
#endif\n\
\n\
uniform sampler2D src_tex_unit0;\n\
uniform vec4 color;\n\
\n\
varying vec2 texCoordVarying;\n\
\n\
void main()\n\
{\n\
	\n\
	vec4 tex = texture2D(src_tex_unit0, texCoordVarying);\n\
	// We will not write anything to the framebuffer if we have a transparent pixel\n\
	// This makes sure we don't mess up our depth buffer.\n\
	if (tex.a < 0.5) discard;\n\
	gl_FragColor = color * tex;\n\
}";

#else

// tig: GLSL #150 shaders written against spec:
// http://www.opengl.org/registry/doc/GLSLangSpec.1.50.09.pdf

string defaultVertexShader =
"#version 150\n\
\n\
uniform mat4 projectionMatrix;\n\
uniform mat4 modelViewMatrix;\n\
uniform mat4 textureMatrix;\
uniform mat4 modelViewProjectionMatrix;\n\
\n\
\n\
in vec4  position;\n\
in vec2  texcoord;\n\
in vec4  color;\n\
in vec3  normal;\n\
\n\
out vec4 colorVarying;\n\
out vec2 texCoordVarying;\n\
out vec4 normalVarying;\n\
\n\
void main()\n\
{\n\
	colorVarying = color;\n\
	texCoordVarying = (textureMatrix*vec4(texcoord.x,texcoord.y,0,1)).xy;\
	gl_Position = modelViewProjectionMatrix * position;\n\
}";

string defaultFragmentShaderTexColor ="\n\
#version 150\n\
\n\
uniform sampler2DRect src_tex_unit0;\n\
uniform float useTexture = 0.0;\n\
uniform float useColors = 0.0;\n\
uniform vec4 color = vec4(1.0);\n\
\n\
in float depth;\n\
in vec4 colorVarying;\n\
in vec2 texCoordVarying;\n\
out vec4 fragColor;\n\
\n\
void main(){\n\
	fragColor = texture(src_tex_unit0, texCoordVarying) * colorVarying;\n\
}";

string defaultFragmentShaderTexNoColor ="\n\
#version 150\n\
\n\
uniform sampler2DRect src_tex_unit0;\n\
uniform float useTexture = 0.0;\n\
uniform float useColors = 0.0;\n\
uniform vec4 color = vec4(1.0);\n\
\n\
in float depth;\n\
in vec4 colorVarying;\n\
in vec2 texCoordVarying;\n\
out vec4 fragColor;\n\
\n\
void main(){\n\
	fragColor = texture(src_tex_unit0, texCoordVarying) * color;\n\
}";

string defaultFragmentShaderTex2DColor ="\n\
#version 150\n\
\n\
uniform sampler2D src_tex_unit0;\n\
uniform float useTexture = 0.0;\n\
uniform float useColors = 0.0;\n\
uniform vec4 color = vec4(1.0);\n\
\n\
in float depth;\n\
in vec4 colorVarying;\n\
in vec2 texCoordVarying;\n\
out vec4 fragColor;\n\
\n\
void main(){\n\
	fragColor = texture(src_tex_unit0, texCoordVarying) * colorVarying;\n\
}";

string defaultFragmentShaderTex2DNoColor ="\n\
#version 150\n\
\n\
uniform sampler2D src_tex_unit0;\n\
uniform float useTexture = 0.0;\n\
uniform float useColors = 0.0;\n\
uniform vec4 color = vec4(1.0);\n\
\n\
in float depth;\n\
in vec4 colorVarying;\n\
in vec2 texCoordVarying;\n\
out vec4 fragColor;\n\
\n\
void main(){\n\
	fragColor = texture(src_tex_unit0, texCoordVarying) * color;\n\
}";

string defaultFragmentShaderNoTexColor ="\n\
#version 150\n\
\n\
uniform sampler2DRect src_tex_unit0;\n\
uniform float useTexture = 0.0;\n\
uniform float useColors = 0.0;\n\
uniform vec4 color = vec4(1.0);\n\
\n\
in float depth;\n\
in vec4 colorVarying;\n\
in vec2 texCoordVarying;\n\
out vec4 fragColor;\n\
\n\
void main(){\n\
	fragColor = colorVarying;\n\
}";

string defaultFragmentShaderNoTexNoColor ="\n\
#version 150\n\
\n\
uniform sampler2DRect src_tex_unit0;\n\
uniform float useTexture = 0.0;\n\
uniform float useColors = 0.0;\n\
uniform vec4 color = vec4(1.0);\n\
\n\
in float depth;\n\
in vec4 colorVarying;\n\
in vec2 texCoordVarying;\n\
out vec4 fragColor;\n\
\n\
void main(){\n\
	fragColor = color;\n\
}";

string bitmapStringVertexShader ="\n\
#version 150\n\
\n\
uniform mat4 projectionMatrix;\n\
uniform mat4 modelViewMatrix;\n\
uniform mat4 textureMatrix;\
uniform mat4 modelViewProjectionMatrix;\n\
\n\
in vec4  position;\n\
in vec4  color;\n\
in vec2  texcoord;\n\
\n\
out vec2 texCoordVarying;\n\
\n\
void main()\n\
{\n\
	texCoordVarying = texcoord;\n\
	gl_Position = modelViewProjectionMatrix * position;\n\
}";

string bitmapStringFragmentShader	= "\n\
#version 150\n\
\n\
uniform sampler2D src_tex_unit0;\n\
uniform vec4 color = vec4(1.0);\n\
in vec2 texCoordVarying;\n\
out vec4 fragColor;\n\
\n\
void main()\n\
{\n\
	\n\
	vec4 tex = texture(src_tex_unit0, texCoordVarying);\n\
	// We will not write anything to the framebuffer if we have a transparent pixel\n\
	// This makes sure we don't mess up our depth buffer.\n\
	if (tex.a < 0.5) discard;\n\
	fragColor = color * tex;\n\
}";
#endif

void ofShader::initDefaultShaders(){
	defaultTexColor().setupShaderFromSource(GL_VERTEX_SHADER,defaultVertexShader);
	defaultTex2DColor().setupShaderFromSource(GL_VERTEX_SHADER,defaultVertexShader);
	defaultNoTexColor().setupShaderFromSource(GL_VERTEX_SHADER,defaultVertexShader);
	defaultTexNoColor().setupShaderFromSource(GL_VERTEX_SHADER,defaultVertexShader);
	defaultTex2DNoColor().setupShaderFromSource(GL_VERTEX_SHADER,defaultVertexShader);
	defaultNoTexNoColor().setupShaderFromSource(GL_VERTEX_SHADER,defaultVertexShader);

#ifndef TARGET_OPENGLES
	defaultTexColor().setupShaderFromSource(GL_FRAGMENT_SHADER,defaultFragmentShaderTexColor);
	defaultTex2DColor().setupShaderFromSource(GL_FRAGMENT_SHADER,defaultFragmentShaderTex2DColor);
#else
	defaultTexColor().setupShaderFromSource(GL_FRAGMENT_SHADER,defaultFragmentShaderTexColor);
	defaultTex2DColor().setupShaderFromSource(GL_FRAGMENT_SHADER,defaultFragmentShaderTexColor);
#endif
	defaultNoTexColor().setupShaderFromSource(GL_FRAGMENT_SHADER,defaultFragmentShaderNoTexColor);
#ifndef TARGET_OPENGLES
	defaultTexNoColor().setupShaderFromSource(GL_FRAGMENT_SHADER,defaultFragmentShaderTexNoColor);
	defaultTex2DNoColor().setupShaderFromSource(GL_FRAGMENT_SHADER,defaultFragmentShaderTex2DNoColor);
#else
	defaultTexNoColor().setupShaderFromSource(GL_FRAGMENT_SHADER,defaultFragmentShaderTexNoColor);
	defaultTex2DNoColor().setupShaderFromSource(GL_FRAGMENT_SHADER,defaultFragmentShaderTexNoColor);
#endif
	defaultNoTexNoColor().setupShaderFromSource(GL_FRAGMENT_SHADER,defaultFragmentShaderNoTexNoColor);


	bitmapStringShader().setupShaderFromSource(GL_VERTEX_SHADER, bitmapStringVertexShader);
	bitmapStringShader().setupShaderFromSource(GL_FRAGMENT_SHADER, bitmapStringFragmentShader);

	defaultTexColor().bindDefaults();
	defaultTex2DColor().bindDefaults();
	defaultNoTexColor().bindDefaults();
	defaultTexNoColor().bindDefaults();
	defaultTex2DNoColor().bindDefaults();
	defaultNoTexNoColor().bindDefaults();

	defaultTexColor().linkProgram();
	defaultTex2DColor().linkProgram();
	defaultNoTexColor().linkProgram();
	defaultTexNoColor().linkProgram();
	defaultTex2DNoColor().linkProgram();
	defaultNoTexNoColor().linkProgram();

	bitmapStringShader().bindDefaults();
	bitmapStringShader().linkProgram();

}

ofShader & ofShader::defaultTexColor(){
	static ofShader * shader = new ofShader;
	return *shader;
}

ofShader & ofShader::defaultTexNoColor(){
	static ofShader * shader = new ofShader;
	return *shader;
}

ofShader & ofShader::defaultTex2DColor(){
	static ofShader * shader = new ofShader;
	return *shader;
}

ofShader & ofShader::defaultTex2DNoColor(){
	static ofShader * shader = new ofShader;
	return *shader;
}

ofShader & ofShader::defaultNoTexColor(){
	static ofShader * shader = new ofShader;
	return *shader;
}

ofShader & ofShader::defaultNoTexNoColor(){
	static ofShader * shader = new ofShader;
	return *shader;
}

ofShader & ofShader::bitmapStringShader(){
	static ofShader * shader = new ofShader;
	return *shader;
}

