
#include "ofxCvGrayscaleImage.h"
#include "ofxCvColorImage.h"
#include "ofxCvFloatImage.h"




//--------------------------------------------------------------------------------
ofxCvFloatImage::ofxCvFloatImage( const ofxCvFloatImage& mom ) {
    allocate(mom.width, mom.height);    
    cvCopy( mom.getCvImage(), cvImage, 0 );
}

//--------------------------------------------------------------------------------
void ofxCvFloatImage::allocate( int w, int h ) {
	if (bAllocated == true){
		cout << "warning: reallocating cvImage in ofxCvFloatImage" << endl;
		clear();
	}
	cvImage = cvCreateImage( cvSize(w,h), IPL_DEPTH_32F, 1 );
	cvImageTemp	= cvCreateImage( cvSize(w,h), IPL_DEPTH_32F, 1 );
	cvGrayscaleImage = NULL;
	pixels = new unsigned char[w*h];
	pixelsAsFloats = NULL;
    width = w;
	height = h;
	bAllocated = true;
    if( bUseTexture ) {
        tex.allocate(width, height, GL_LUMINANCE);
        bTextureDirty = true;
    }
}

//--------------------------------------------------------------------------------
void ofxCvFloatImage::clear() {
    if (bAllocated == true){
        if( cvGrayscaleImage != NULL ){
            cvReleaseImage( &cvGrayscaleImage );
        }
        if( pixelsAsFloats != NULL ) {
            delete pixelsAsFloats;
            pixelsAsFloats = NULL;
        }
    }
    ofxCvImage::clear();    //call clear in base class    
}



// Set Pixel Data

//-------------------------------------------------------------------------------------
void ofxCvFloatImage::set(float value){  
	cvSet(cvImage, cvScalar(value));
    bTextureDirty = true;
}

//--------------------------------------------------------------------------------
void ofxCvFloatImage::setFromPixels( unsigned char * _pixels, int w, int h, float scaleMin, float scaleMax ) {
    if( cvGrayscaleImage == NULL ) {
        cvGrayscaleImage = cvCreateImage( cvSize(width,height), IPL_DEPTH_8U, 1 );
    }
    for( int i = 0; i < h; i++ ) {
		memcpy( cvGrayscaleImage->imageData+(i*cvGrayscaleImage->widthStep), _pixels+(i*w), w );
	}
    cvConvert( cvGrayscaleImage, cvImage );
    bTextureDirty = true;
}

//--------------------------------------------------------------------------------
void ofxCvFloatImage::setFromPixels( float * _pixels, int w, int h ) {
	for( int i = 0; i < h; i++ ) {
		memcpy( cvImage->imageData+(i*cvImage->widthStep), _pixels+(i*w), w*sizeof(float));
	}
    bTextureDirty = true;
}

//--------------------------------------------------------------------------------
void ofxCvFloatImage::operator *= ( float scalar ){
    // does this exist in opencv?
    int totalPixels = cvImage->width*cvImage->height;
    if(totalPixels <= 0)return;

    float * ptr = ((float *)cvImage->imageData);

    for (int i = 0; i < totalPixels; i++){
       ( *ptr ) *= scalar;
        if( ( *ptr ) < 0.00001 ) (*ptr) = 0.0;
        ptr++;
    }
    
    bTextureDirty = true;
}

//--------------------------------------------------------------------------------
void ofxCvFloatImage::operator /= ( float scalar ){
    // does this exist in opencv?
    int totalPixels = cvImage->width*cvImage->height;
    if(totalPixels == 0 || scalar == 0)return;

    //inverse and then multiply
    scalar = 1.0 / scalar;

    float * ptr = ((float *)cvImage->imageData);

    for (int i = 0; i < totalPixels; i++){
       ( *ptr ) *= scalar;
        if( ( *ptr ) < 0.00001 ) (*ptr) = 0.0;
        ptr++;
    }
    
    bTextureDirty = true;
}

//--------------------------------------------------------------------------------
void ofxCvFloatImage::operator = ( const ofxCvGrayscaleImage& mom ) {
	if( mom.width == width && mom.height == height ) {
        cvConvert( mom.getCvImage(), cvImage );
        bTextureDirty = true;
	} else {
        cout << "error in =, images are different sizes" << endl;
	}
}

//--------------------------------------------------------------------------------
void ofxCvFloatImage::operator = ( const ofxCvColorImage& mom ) {
	if( mom.width == width && mom.height == height ) {
        if( cvGrayscaleImage == NULL ) {
            cvGrayscaleImage = cvCreateImage( cvSize(width,height), IPL_DEPTH_8U, 1 );
        }    
		cvCvtColor( mom.getCvImage(), cvGrayscaleImage, CV_RGB2GRAY );
        cvConvert( cvGrayscaleImage, cvImage );
        bTextureDirty = true;
	} else {
        cout << "error in =, images are different sizes" << endl;
	}
}

//--------------------------------------------------------------------------------
void ofxCvFloatImage::operator = ( const ofxCvFloatImage& mom ) {
	if( mom.width == width && mom.height == height ) {
		cvCopy( mom.getCvImage(), cvImage, 0 );
        bTextureDirty = true;
	} else {
        cout << "error in =, images are different sizes" << endl;
	}
}

//--------------------------------------------------------------------------------
void ofxCvFloatImage::operator *= ( const ofxCvImage& mom ) {
	if( mom.width == width && mom.height == height &&
	    mom.getCvImage()->nChannels == cvImage->nChannels && 
        mom.getCvImage()->depth == cvImage->depth )
    {
		cvMul( cvImage, mom.getCvImage(), cvImageTemp );
		swapTemp();
        bTextureDirty = true;
	} else {
        cout << "error in *=, images need to match in size and type" << endl;
	}
}

//--------------------------------------------------------------------------------
void ofxCvFloatImage::operator &= ( const ofxCvImage& mom ) {
	if( mom.width == width && mom.height == height &&
	    mom.getCvImage()->nChannels == cvImage->nChannels && 
        mom.getCvImage()->depth == cvImage->depth )
    {
	    //this is doing it bit-wise; probably not what we want
		cvAnd( cvImage, mom.getCvImage(), cvImageTemp );
		swapTemp();
        bTextureDirty = true;
	} else {
        cout << "error in &=, images need to match in size and type" << endl;
	}
}


//--------------------------------------------------------------------------------
void ofxCvFloatImage::addWeighted( ofxCvGrayscaleImage& mom, float f ) {
	if( mom.width == width && mom.height == height ) {
         IplImage* cvTemp = cvCreateImage( cvSize(width,height), IPL_DEPTH_32F, 1 );
         cvConvertScale( mom.getCvImage(), cvTemp, 1, 0 );
         cvAddWeighted( cvTemp, f, cvImage, 1.0f-f,0, cvImageTemp );
         swapTemp();
         bTextureDirty = true;
         cvReleaseImage( &cvTemp );
    } else {
        cout << "error in addWeighted, images are different sizes" << endl;
    }
}



// Get Pixel Data

//--------------------------------------------------------------------------------
unsigned char*  ofxCvFloatImage::getPixels(float scaleMin, float scaleMax){
    float range = (scaleMax - scaleMin);
    float scale = 255/range;
    float offset = - (scaleMin * scale);  // ie, 0.5 - 1 = scale by (255*2), subtract 255, 128-255 = scale by 1/2, subtract 128
    if( cvGrayscaleImage == NULL ) {
        cvGrayscaleImage = cvCreateImage( cvSize(width,height), IPL_DEPTH_8U, 1 );
    }    
    cvConvertScale( cvImage, cvGrayscaleImage, scale, offset );
    for( int i = 0; i < height; i++ ) {
		memcpy( pixels+(i*width),
                cvGrayscaleImage->imageData+(i*cvGrayscaleImage->widthStep), width );
	}
	return pixels;
}

//--------------------------------------------------------------------------------
float * ofxCvFloatImage::getPixelsAsFloats(){
    // ok??
    if( cvGrayscaleImage == NULL ) {
        cvGrayscaleImage = cvCreateImage( cvSize(width,height), IPL_DEPTH_8U, 1 );
    }
    if( pixelsAsFloats == NULL ) {
        pixelsAsFloats = new float[width*height];
    }
    for( int i = 0; i < height; i++ ) {
		memcpy( pixelsAsFloats+(i*width),
                cvGrayscaleImage->imageData+(i*(cvGrayscaleImage->widthStep / 4)), width*4 );
	}
	return pixelsAsFloats;
}



// Draw Image

//--------------------------------------------------------------------------------
void ofxCvFloatImage::drawWithScale( float x, float y, float scaleMin, float scaleMax){
    drawWithScale(x,y,width, height, scaleMin, scaleMax);
}

//--------------------------------------------------------------------------------
void ofxCvFloatImage::drawWithScale( float x, float y, float w, float h, float scaleMin, float scaleMax){
    if( bUseTexture ) {
        // note, this is a bit ineficient, as we have to
        // copy the data out of the cvImage into the pixel array
        // and then upload to texture.  We should add
        // to the texture class an override for pixelstorei
        // that allows stepped-width image upload:
        if( bTextureDirty ) {
            tex.loadData(getPixels(scaleMin, scaleMax), width, height, GL_LUMINANCE);
            bTextureDirty = false;
        }        
        tex.draw(x,y,w,h);
    }
}




// Image Transformation Operations

//--------------------------------------------------------------------------------
void ofxCvFloatImage::resize( int w, int h ) {

    // note, one image copy operation could be ommitted by
    // reusing the temporal image storage

    IplImage* temp = cvCreateImage( cvSize(w,h), IPL_DEPTH_32F, 1 );
    cvResize( cvImage, temp );
    clear();
    allocate( w, h );
    cvCopy( temp, cvImage );
    cvReleaseImage( &temp );
}

//--------------------------------------------------------------------------------
void ofxCvFloatImage::scaleIntoMe( const ofxCvImage& mom, int interpolationMethod ){
    //for interpolation you can pass in:
    //CV_INTER_NN - nearest-neigbor interpolation,
    //CV_INTER_LINEAR - bilinear interpolation (used by default)
    //CV_INTER_AREA - resampling using pixel area relation. It is preferred method 
    //                for image decimation that gives moire-free results. In case of 
    //                zooming it is similar to CV_INTER_NN method.
    //CV_INTER_CUBIC - bicubic interpolation.
        
    if( mom.getCvImage()->nChannels == cvImage->nChannels && 
        mom.getCvImage()->depth == cvImage->depth ) {
    
        if ((interpolationMethod != CV_INTER_NN) &&
            (interpolationMethod != CV_INTER_LINEAR) &&
            (interpolationMethod != CV_INTER_AREA) &&
            (interpolationMethod != CV_INTER_CUBIC) ){
            cout << "error in scaleIntoMe / interpolationMethod, setting to CV_INTER_NN" << endl;
    		interpolationMethod = CV_INTER_NN;
    	}
        cvResize( mom.getCvImage(), cvImage, interpolationMethod );
        bTextureDirty = true;

    } else {
        cout << "error in scaleIntoMe: mom image type has to match" << endl;
    }
}
