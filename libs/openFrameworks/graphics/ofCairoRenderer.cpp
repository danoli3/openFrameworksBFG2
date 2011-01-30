/*
 * ofCairoRenderer.cpp
 *
 *  Created on: 26/01/2011
 *      Author: arturo
 */

#include "ofCairoRenderer.h"
#include "ofGraphics.h"
#include "ofConstants.h"
#include "ofAppRunner.h"

void
helper_quadratic_to (cairo_t *cr,
                     double x1, double y1,
                     double x2, double y2)
{
  double x0, y0;
  cairo_get_current_point (cr, &x0, &y0);
  cairo_curve_to (cr,
                  2.0 / 3.0 * x1 + 1.0 / 3.0 * x0,
                  2.0 / 3.0 * y1 + 1.0 / 3.0 * y0,
                  2.0 / 3.0 * x1 + 1.0 / 3.0 * x2,
                  2.0 / 3.0 * y1 + 1.0 / 3.0 * y2,
                  y1, y2);
}

ofVertexData get_triangles(const ofVertexData & vertexData){
	ofVertexData triangles;

	switch(vertexData.getMode()){
	case(OF_TRIANGLES_MODE):
		for(int i = 0; i < vertexData.getNumVertices();i++){
			triangles.addIndex((GLuint)i);
	}
	break;
	case(OF_TRIANGLE_STRIP_MODE):
		triangles.addIndex((GLuint)0);
		triangles.addIndex((GLuint)1);
		triangles.addIndex((GLuint)2);
		for(int i = 3; i < vertexData.getNumVertices();i++){
			triangles.addIndex((GLuint)i);
			triangles.addIndex((GLuint)i-2);
		}
	break;
	case(OF_TRIANGLE_FAN_MODE):
		triangles.addIndex((GLuint)0);
		triangles.addIndex((GLuint)1);
		triangles.addIndex((GLuint)2);
		for(int i = 2; i < vertexData.getNumVertices()-1;i++){
			triangles.addIndex((GLuint)0);
			triangles.addIndex((GLuint)i);
			triangles.addIndex((GLuint)i+1);
		}
	break;
	default:break;
	}
	return triangles;
}

ofCairoRenderer::ofCairoRenderer(){
	type = PDF;
	surface = NULL;
	cr = NULL;
}

ofCairoRenderer::~ofCairoRenderer(){
	close();
}

void ofCairoRenderer::setup(string filename, Type type){
	switch(type){
	case PDF:
		surface = cairo_pdf_surface_create(ofToDataPath(filename).c_str(),ofGetWidth(),ofGetHeight());
		break;
	case SVG:
		surface = cairo_svg_surface_create(ofToDataPath(filename).c_str(),ofGetWidth(),ofGetHeight());
		break;
	}

	cr = cairo_create(surface);
	page = 0;
}

void ofCairoRenderer::close(){
	if(surface){
		cairo_surface_flush(surface);
		cairo_surface_finish(surface);
		cairo_surface_destroy(surface);
		surface = NULL;
	}
	if(cr){
		cairo_destroy(cr);
		cr = NULL;
	}
}

void ofCairoRenderer::draw(ofShape & path){
	drawPath(path);
}

void ofCairoRenderer::draw(ofPolyline & poly){
	ofPushStyle();
	cairo_new_path(cr);
	for(int i=0;i<poly.size();i++){
		cairo_line_to(cr,poly.getVertices()[i].x,poly.getVertices()[i].y);
	}
	if(poly.isClosed())
		cairo_close_path(cr);
	ofPopStyle();
}

void ofCairoRenderer::draw(ofShapeTessellation & shape){
	for(int i=0;i<shape.getTessellation().size();i++){
		draw(shape.getTessellation()[i]);
	}
	for(int i=0;i<shape.getOutline().size();i++){
		draw(shape.getOutline()[i]);
	}
}

void ofCairoRenderer::draw(ofVertexData & vertexData){
	if(vertexData.getNumVertices()==0) return;
	ofVertexData indices = get_triangles(vertexData);
	if(indices.getNumIndices()<4) return;
	cairo_new_sub_path(cr);
	//if(indices.getNumIndices()){

		int i = 0;
		if(vertexData.getMode()==OF_TRIANGLE_STRIP_MODE){
			cairo_move_to(cr,vertexData.getVertex(vertexData.getIndex(0)).x,vertexData.getVertex(vertexData.getIndex(0)).y);
			cairo_line_to(cr,vertexData.getVertex(vertexData.getIndex(1)).x,vertexData.getVertex(vertexData.getIndex(1)).y);
			cairo_line_to(cr,vertexData.getVertex(vertexData.getIndex(2)).x,vertexData.getVertex(vertexData.getIndex(2)).y);
			i=2;
		}
		for(; i<vertexData.getNumIndices(); i++){
			ofVec3f transformed=projection.preMult(vertexData.getVertex(vertexData.getIndex(i)));
			transformed=modelView.preMult(transformed);
			transformed.set(transformed.x/transformed.z,transformed.y/transformed.z);
			switch(vertexData.getMode()){
			case(OF_TRIANGLES_MODE):
				cairo_move_to(cr,vertexData.getVertex(vertexData.getIndex(i)).x,vertexData.getVertex(vertexData.getIndex(i)).y);
				if((i+1)%3==0){

					ofVec3f transformed_2=projection.preMult(vertexData.getVertex(vertexData.getIndex(i-2)));
					transformed_2=modelView.preMult(transformed_2);
					transformed_2.set(transformed_2.x/transformed_2.z,transformed_2.y/transformed_2.z);

					cairo_line_to(cr,transformed_2.x,transformed_2.y);
					cairo_move_to(cr,transformed.x,transformed.y);
				}

			break;
			case(OF_TRIANGLE_STRIP_MODE):{

					ofVec3f transformed_2=projection.preMult(vertexData.getVertex(vertexData.getIndex(i-2)));
					transformed_2=modelView.preMult(transformed_2);
					transformed_2.set(transformed_2.x/transformed_2.z,transformed_2.y/transformed_2.z);

					cairo_line_to(cr,transformed.x,transformed.y);
					cairo_line_to(cr,transformed_2.x,transformed_2.y);
					cairo_move_to(cr,transformed.x,transformed.y);
			}break;
			case(OF_TRIANGLE_FAN_MODE):
			break;
			default:break;
			}
			/*switch(vertexData.getMode()){
			case(OF_TRIANGLES_MODE):
				cairo_move_to(cr,vertexData.getVertex(vertexData.getIndex(i)).x,vertexData.getVertex(vertexData.getIndex(i)).y);
				if((i+1)%3==0){
					cairo_line_to(cr,vertexData.getVertex(vertexData.getIndex(i-2)).x,vertexData.getVertex(vertexData.getIndex(i-2)).y);
					cairo_move_to(cr,vertexData.getVertex(vertexData.getIndex(i)).x,vertexData.getVertex(vertexData.getIndex(i)).y);
				}

			break;
			case(OF_TRIANGLE_STRIP_MODE):
					cairo_line_to(cr,vertexData.getVertex(vertexData.getIndex(i)).x,vertexData.getVertex(vertexData.getIndex(i)).y);
					cairo_line_to(cr,vertexData.getVertex(vertexData.getIndex(i-2)).x,vertexData.getVertex(vertexData.getIndex(i-2)).y);
					cairo_move_to(cr,vertexData.getVertex(vertexData.getIndex(i)).x,vertexData.getVertex(vertexData.getIndex(i)).y);
			break;
			case(OF_TRIANGLE_FAN_MODE):
			break;
			default:break;
			}*/
		}
			/**/

	/*}else{
		for(int i=0; i<vertexData.getNumVertices(); i++){
			cairo_line_to(cr,vertexData.getVertex(i).x,vertexData.getVertex(i).y);
		}
	}*/
	if(ofGetStyle().lineWidth>0){
		ofColor c = ofGetStyle().color;
		cairo_set_source_rgba(cr, (float)c.r/255.0, (float)c.g/255.0, (float)c.b/255.0, (float)c.a/255.0);
		cairo_set_line_width( cr, ofGetStyle().lineWidth );
		/*if(ofGetStyle().)
			cairo_stroke_preserve( cr );
		else
			cairo_stroke( cr );*/

		cairo_stroke( cr );
	}
}

void ofCairoRenderer::drawPath(const ofShape & path,bool is_subpath){
	if(!surface || !cr) return;
	const vector<ofShape::Command> & commands = path.getCommands();
	if(is_subpath)
		cairo_new_sub_path(cr);
	else
		cairo_new_path(cr);
	for(int i=0; i<commands.size(); i++){
		switch(commands[i].type){
		case ofShape::Command::line2DTo:
		case ofShape::Command::line3DTo:
			curvePoints.clear();
			cairo_line_to(cr,commands[i].to.x,commands[i].to.y);
			break;


		case ofShape::Command::curve2DTo:
			curvePoints.push_back(commands[i].to);

			//code adapted from ofxVectorGraphics to convert catmull rom to bezier
			if(curvePoints.size()==4){
				ofPoint p1=curvePoints[0];
				ofPoint p2=curvePoints[1];
				ofPoint p3=curvePoints[2];
				ofPoint p4=curvePoints[3];

				//SUPER WEIRD MAGIC CONSTANT = 1/6 (this works 100% can someone explain it?)
				ofPoint cp1 = p2 + ( p3 - p1 ) * (1.0/6);
				ofPoint cp2 = p3 + ( p2 - p4 ) * (1.0/6);

				cairo_curve_to( cr, cp1.x, cp1.y, cp2.x, cp2.y, p3.x, p3.y );
				curvePoints.pop_front();
			}
			break;


		case ofShape::Command::bezier2DTo:
			curvePoints.clear();
			cairo_curve_to(cr,commands[i].cp1.x,commands[i].cp1.y,commands[i].cp2.x,commands[i].cp2.y,commands[i].to.x,commands[i].to.y);
			break;

		case ofShape::Command::quadBezier2DTo:
			curvePoints.clear();
			cairo_curve_to(cr,commands[i].cp1.x,commands[i].cp1.y,commands[i].cp2.x,commands[i].cp2.y,commands[i].to.x,commands[i].to.y);
			break;


		case ofShape::Command::arc2D:
			curvePoints.clear();
			// elliptic arcs not directly supported in cairo, lets scale y
			if(commands[i].radiusX!=commands[i].radiusY){
				float ellipse_ratio = commands[i].radiusY/commands[i].radiusX;
				/*cairo_matrix_t current_matrix;
				cairo_get_matrix(cr,&current_matrix);
				cairo_matrix_t stored_matrix = current_matrix;
				cairo_matrix_translate(&current_matrix,0,-commands[i].to.y*ellipse_ratio);
				cairo_matrix_scale(&current_matrix,1,ellipse_ratio);
				cairo_matrix_translate(&current_matrix,0,commands[i].to.y*1/ellipse_ratio);
				cairo_set_matrix(cr,&current_matrix);*/
				pushMatrix();
				translate(0,-commands[i].to.y*ellipse_ratio);
				scale(1,ellipse_ratio);
				translate(0,commands[i].to.y*1/ellipse_ratio);
				cairo_arc(cr,commands[i].to.x,commands[i].to.y,commands[i].radiusX,commands[i].angleBegin,commands[i].angleEnd);
				//cairo_set_matrix(cr,&stored_matrix);
				popMatrix();
			}else{
				cairo_arc(cr,commands[i].to.x,commands[i].to.y,commands[i].radiusX,commands[i].angleBegin,commands[i].angleEnd);
			}
			break;
		}
	}

	if(path.isClosed()){
		cairo_close_path(cr);
	}

	const vector<ofShape> &subpaths = path.getSubShapes();
	for(int i=0;i<(int)subpaths.size();i++){
		drawPath(subpaths[i],true);
	}

	cairo_fill_rule_t cairo_poly_mode;
	if(path.getWindingMode()==OF_POLY_WINDING_ODD) cairo_poly_mode=CAIRO_FILL_RULE_EVEN_ODD;
	else cairo_poly_mode=CAIRO_FILL_RULE_WINDING;

	cairo_set_fill_rule(cr,cairo_poly_mode);


	if(path.getStrokeWidth()>0){
		ofColor c = path.getStrokeColor() * ofGetStyle().color;
		cairo_set_source_rgba(cr, (float)c.r/255.0, (float)c.g/255.0, (float)c.b/255.0, (float)c.a/255.0);
		cairo_set_line_width( cr, path.getStrokeWidth() );
		if(path.isFilled())
			cairo_stroke_preserve( cr );
		else
			cairo_stroke( cr );
	}
	if(path.isFilled()){
		ofColor c = path.getFillColor() * ofGetStyle().color;
		cairo_set_source_rgba(cr, (float)c.r/255.0, (float)c.g/255.0, (float)c.b/255.0, (float)c.a/255.0);
		cairo_fill( cr );
	}
}

//--------------------------------------------
// transformations
//our openGL wrappers
void ofCairoRenderer::pushMatrix(){
	if(!surface || !cr) return;
	cairo_get_matrix(cr,&tmpMatrix);
	matrixStack.push(tmpMatrix);
}

void ofCairoRenderer::popMatrix(){
	if(!surface || !cr) return;
	cairo_set_matrix(cr,&matrixStack.top());
	matrixStack.pop();
}

void ofCairoRenderer::translate(float x, float y, float z ){
	if(!surface || !cr) return;
	cairo_get_matrix(cr,&tmpMatrix);
	cairo_matrix_translate(&tmpMatrix,x,y);
	cairo_set_matrix(cr,&tmpMatrix);
}

void ofCairoRenderer::translate(const ofPoint & p){
	translate(p.x,p.y,p.z);
}

void ofCairoRenderer::scale(float xAmnt, float yAmnt, float zAmnt ){
	if(!surface || !cr) return;
	cairo_get_matrix(cr,&tmpMatrix);
	cairo_matrix_scale(&tmpMatrix,xAmnt,yAmnt);
	cairo_set_matrix(cr,&tmpMatrix);
}

void ofCairoRenderer::rotateZ(float degrees){
	if(!surface || !cr) return;
	cairo_get_matrix(cr,&tmpMatrix);
	cairo_matrix_rotate(&tmpMatrix,degrees*DEG_TO_RAD);
	cairo_set_matrix(cr,&tmpMatrix);
}

void ofCairoRenderer::rotate(float degrees){
	rotateZ(degrees);
}


void ofCairoRenderer::setupScreen(){
	if(!surface || !cr) return;
	if(page==0){
		page=1;
		return;
	}else{
		page++;
		if(ofbClearBg()){
			cairo_show_page(cr);
		}else{
			cairo_copy_page(cr);
		}
	}
	ofSetupScreenPerspective();	// assume defaults
}

// screen coordinate things / default gl values
void ofCairoRenderer::pushView(){}
void ofCairoRenderer::popView(){};

// setup matrices and viewport (upto you to push and pop view before and after)
// if width or height are 0, assume windows dimensions (ofGetWidth(), ofGetHeight())
// if nearDist or farDist are 0 assume defaults (calculated based on width / height)
void ofCairoRenderer::viewport(float x, float y, float width, float height, bool invertY){

};

void ofCairoRenderer::setupScreenPerspective(float width, float height, int orientation, bool vFlip, float fov, float nearDist, float farDist){
	vFlip = !vFlip;
	if(width == 0) width = ofGetWidth();
	if(height == 0) height = ofGetHeight();
	if( orientation == 0 ) orientation = ofGetOrientation();

	float w = width;
	float h = height;

	//we do this because ofGetWidth and ofGetHeight return orientated widths and height
	//for the camera we need width and height of the actual screen
	if( orientation == OF_ORIENTATION_90_LEFT || orientation == OF_ORIENTATION_90_RIGHT ){
		h = width;
		w = height;
	}

	float eyeX = w / 2;
	float eyeY = h / 2;
	float halfFov = PI * fov / 360;
	float theTan = tanf(halfFov);
	float dist = eyeY / theTan;
	float aspect = (float) w / h;

	if(nearDist == 0) nearDist = dist / 10.0f;
	if(farDist == 0) farDist = dist * 10.0f;

	projection.makePerspectiveMatrix(fov,aspect,nearDist,farDist);
	//gluPerspective(fov, aspect, nearDist, farDist);

	//glMatrixMode(GL_MODELVIEW);
	//glLoadIdentity();
	//gluLookAt(eyeX, eyeY, dist, eyeX, eyeY, 0, 0, 1, 0);
	modelView.makeLookAtMatrix(ofVec3f(eyeX,eyeY,dist),ofVec3f(eyeX,eyeY,0),ofVec3f(0,1,0));

	//note - theo checked this on iPhone and Desktop for both vFlip = false and true
	switch(orientation) {
		case OF_ORIENTATION_180:
			modelView.rotate(-180,0,0,1);
			//glRotatef(-180, 0, 0, 1);
			if(vFlip){
				modelView.scale(1,-1,1);
				modelView.translate(-width,0,0);
				//glScalef(1, -1, 1);
				//glTranslatef(-width, 0, 0);
			}else{
				modelView.translate(-width,-height,0);
				//glTranslatef(-width, -height, 0);
			}

			break;

		case OF_ORIENTATION_90_RIGHT:
			//glRotatef(-90, 0, 0, 1);
			modelView.rotate(-90,0,0,1);
			if(vFlip){
				//glScalef(-1, 1, 1);
				modelView.scale(-1,1,1);
			}else{
				//glScalef(-1, -1, 1);
				//glTranslatef(0, -height, 0);
				modelView.scale(-1,-1,1);
				modelView.translate(0,-height,0);
			}
			break;

		case OF_ORIENTATION_90_LEFT:
			//glRotatef(90, 0, 0, 1);
			modelView.rotate(90,0,0,1);
			if(vFlip){
				//glScalef(-1, 1, 1);
				//glTranslatef(-width, -height, 0);
				modelView.scale(-1,1,1);
				modelView.translate(-width,-height,0);
			}else{
				//glScalef(-1, -1, 1);
				//glTranslatef(-width, 0, 0);

				modelView.scale(-1,-1,1);
				modelView.translate(-width,0,0);
			}
			break;

		case OF_ORIENTATION_DEFAULT:
		default:
			if(vFlip){
				//glScalef(1, -1, 1);
				//glTranslatef(0, -height, 0);
				modelView.scale(1,-1,1);
				modelView.translate(0,-height,0);
			}
			break;
	}
};

void ofCairoRenderer::setupScreenOrtho(float width, float height, bool vFlip, float nearDist, float farDist){};

ofRectangle ofCairoRenderer::getCurrentViewport(){return ofRectangle();};
int ofCairoRenderer::getViewportWidth(){return ofGetWidth();};
int ofCairoRenderer::getViewportHeight(){return ofGetHeight();};

void ofCairoRenderer::setCoordHandedness(ofHandednessType handedness){};
ofHandednessType ofCairoRenderer::getCoordHandedness(){return OF_LEFT_HANDED;};

void ofCairoRenderer::setupGraphicDefaults(){};


void ofCairoRenderer::rotate(float degrees, float vecX, float vecY, float vecZ){}
void ofCairoRenderer::rotateX(float degrees){}
void ofCairoRenderer::rotateY(float degrees){}


cairo_t * ofCairoRenderer::getCairoContext(){
	return cr;
}

cairo_surface_t * ofCairoRenderer::getCairoSurface(){
	return surface;
}
