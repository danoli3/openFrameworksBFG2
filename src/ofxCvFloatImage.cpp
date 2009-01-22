
#include "ofxCvGrayscaleImage.h"
#include "ofxCvColorImage.h"
#include "ofxCvFloatImage.h"




//--------------------------------------------------------------------------------
ofxCvFloatImage::ofxCvFloatImage() {
    ipldepth = IPL_DEPTH_32F;
    iplchannels = 1;
    gldepth = GL_FLOAT;
    glchannels = GL_LUMINANCE;
    floatPixels = NULL; 
    bFloatPixelsDirty = true;
    floatPixelsW = 0;
    floatPixelsH = 0;   
    cvGrayscaleImage = NULL;
    scaleMin = 0.0f;
    scaleMax = 1.0f;
}

//--------------------------------------------------------------------------------
ofxCvFloatImage::ofxCvFloatImage( const ofxCvFloatImage& _mom ) {
    // cast non-const,  to get read access to the mon::cvImage
    ofxCvFloatImage& mom = const_cast<ofxCvFloatImage&>(_mom); 
    allocate(mom.width, mom.height);    
    cvCopy( mom.getCvImage(), cvImage, 0 );
}


//--------------------------------------------------------------------------------
void ofxCvFloatImage::clear() {
    if (bAllocated == true){
        if( cvGrayscaleImage != NULL ){
            cvReleaseImage( &cvGrayscaleImage );
        }
        if( floatPixels != NULL ) {
            delete floatPixels;
            floatPixels = NULL;
            bFloatPixelsDirty = true;
            floatPixelsW = 0;
            floatPixelsH = 0;             
        }
    }
    ofxCvImage::clear();    //call clear in base class    
}

//--------------------------------------------------------------------------------
void ofxCvFloatImage::setNativeScale( float _scaleMin, float _scaleMax ) {
    scaleMin = _scaleMin;
    scaleMax = _scaleMax;
}

//--------------------------------------------------------------------------------
void ofxCvFloatImage::flagImageChanged() {
    bFloatPixelsDirty = true;
    ofxCvImage::flagImageChanged();
}

//--------------------------------------------------------------------------------
void ofxCvFloatImage::convertFloatToGray( IplImage* floatImg, IplImage* grayImg ) {
    // map from scaleMin-scaleMax to 0-255
    float scale = 255.0f/(scaleMax-scaleMin);
    cvConvertScale( floatImg, grayImg, scale, -(scaleMin*scale) );
}

//--------------------------------------------------------------------------------
void ofxCvFloatImage::convertGrayToFloat( IplImage* grayImg, IplImage* floatImg ) {
    // map from 0-255 to scaleMin-scaleMax
    cvConvertScale( grayImg, floatImg, (scaleMax-scaleMin)/255.0f, scaleMin );
}




// Set Pixel Data

//-------------------------------------------------------------------------------------
void ofxCvFloatImage::set(float value){  
	cvSet(cvImage, cvScalar(value));
    flagImageChanged();
}

//--------------------------------------------------------------------------------
void ofxCvFloatImage::operator *= ( float scalar ){
    for( int i=0; i<height; i++ ) { 
        float* ptr = (float*)(cvImage->imageData + (i+roiY)*cvImage->widthStep); 
        for( int j=0; j<width; j++ ) { 
            ptr[(j+roiX)] *= scalar; 
        } 
    } 
}

//--------------------------------------------------------------------------------
void ofxCvFloatImage::operator /= ( float scalar ){
    scalar = 1.0 / scalar;
    for( int i=0; i<height; i++ ) { 
        float* ptr = (float*)(cvImage->imageData + (i+roiY)*cvImage->widthStep); 
        for( int j=0; j<width; j++ ) { 
            ptr[(j+roiX)] *= scalar; 
        } 
    } 
}

//--------------------------------------------------------------------------------
void ofxCvFloatImage::setFromPixels( unsigned char* _pixels, int w, int h ) {    
    if( cvGrayscaleImage == NULL ) {
        cvGrayscaleImage = cvCreateImage( cvSize(cvImage->width,cvImage->height), IPL_DEPTH_8U, 1 );
    }
    
    cvSetImageROI(cvGrayscaleImage, cvRect(roiX,roiY,width,height));  //make sure ROI is in sync

    ofRectangle roi = getROI();
    ofRectangle inputROI = ofRectangle( roi.x, roi.y, w, h);
    ofRectangle iRoi = getIntersectionROI( roi, inputROI );
        
    if( iRoi.width > 0 && iRoi.height > 0 ) {
        // copy pixels from _pixels, however many we have or will fit in cvGrayscaleImage
        for( int i=0; i < iRoi.height; i++ ) {
            memcpy( cvGrayscaleImage->imageData + ((i+(int)iRoi.y)*cvGrayscaleImage->widthStep) + (int)iRoi.x,
                    _pixels + (i*w),
                    iRoi.width );
        }
        convertGrayToFloat(cvGrayscaleImage, cvImage);
        flagImageChanged();
    } else {
        ofLog(OF_ERROR, "in setFromPixels, ROI mismatch");
    }    
}

//--------------------------------------------------------------------------------
void ofxCvFloatImage::setFromPixels( float* _pixels, int w, int h ) {
    ofRectangle roi = getROI();
    ofRectangle inputROI = ofRectangle( roi.x, roi.y, w, h);
    ofRectangle iRoi = getIntersectionROI( roi, inputROI );
        
    if( iRoi.width > 0 && iRoi.height > 0 ) {
        // copy pixels from _pixels, however many we have or will fit in cvImage
        for( int i=0; i < iRoi.height; i++ ) {
            memcpy( cvImage->imageData + ((i+(int)iRoi.y)*cvImage->widthStep) + (int)iRoi.x*sizeof(float),
                    _pixels + (i*w),
                    iRoi.width*sizeof(float) );
        }
        flagImageChanged();
    } else {
        ofLog(OF_ERROR, "in setFromPixels, ROI mismatch");
    }     
}

//--------------------------------------------------------------------------------
void ofxCvFloatImage::operator = ( unsigned char* _pixels ) {
    setFromPixels( _pixels, width, height );
}

//--------------------------------------------------------------------------------
void ofxCvFloatImage::operator = ( float* _pixels ) {
    setFromPixels( _pixels, width, height );
}

//--------------------------------------------------------------------------------
void ofxCvFloatImage::operator = ( const ofxCvGrayscaleImage& _mom ) {
    // cast non-const,  no worries, we will reverse any chages
    ofxCvGrayscaleImage& mom = const_cast<ofxCvGrayscaleImage&>(_mom); 
	if( pushSetBothToTheirIntersectionROI(*this,mom) ) {
        convertGrayToFloat(mom.getCvImage(), cvImage);
        popROI();       //restore prevoius ROI
        mom.popROI();   //restore prevoius ROI              
        flagImageChanged();
	} else {
        ofLog(OF_ERROR, "in =, images are different sizes");
	}
}

//--------------------------------------------------------------------------------
void ofxCvFloatImage::operator = ( const ofxCvColorImage& _mom ) {
    // cast non-const,  no worries, we will reverse any chages
    ofxCvColorImage& mom = const_cast<ofxCvColorImage&>(_mom); 
	if( pushSetBothToTheirIntersectionROI(*this,mom) ) {
        if( cvGrayscaleImage == NULL ) {
            cvGrayscaleImage = cvCreateImage( cvSize(cvImage->width,cvImage->height), IPL_DEPTH_8U, 1 );
        }
        cvSetImageROI(cvGrayscaleImage, cvRect(roiX,roiY,width,height));
		cvCvtColor( mom.getCvImage(), cvGrayscaleImage, CV_RGB2GRAY );
        convertGrayToFloat(cvGrayscaleImage, cvImage);                
        popROI();       //restore prevoius ROI
        mom.popROI();   //restore prevoius ROI   
        cvSetImageROI(cvGrayscaleImage, cvRect(roiX,roiY,width,height));
        flagImageChanged();
	} else {
        ofLog(OF_ERROR, "in =, images are different sizes");
	}
}

//--------------------------------------------------------------------------------
void ofxCvFloatImage::operator = ( const ofxCvFloatImage& _mom ) {
    if(this != &_mom) {  //check for self-assignment
        // cast non-const,  no worries, we will reverse any chages
        ofxCvFloatImage& mom = const_cast<ofxCvFloatImage&>(_mom);     
        if( pushSetBothToTheirIntersectionROI(*this,mom) ) {
            cvCopy( mom.getCvImage(), cvImage, 0 );
            popROI();       //restore prevoius ROI
            mom.popROI();   //restore prevoius ROI             
            flagImageChanged();
        } else {
            ofLog(OF_ERROR, "in =, images are different sizes");
        }
    } else {
        ofLog(OF_WARNING, "in =, you are assigning a ofxCvFloatImage to itself");
    }
}

//--------------------------------------------------------------------------------
void ofxCvFloatImage::operator *= ( ofxCvImage& mom ) {
	if( mom.getCvImage()->nChannels == cvImage->nChannels && 
        mom.getCvImage()->depth == cvImage->depth )
    {
        if( pushSetBothToTheirIntersectionROI(*this,mom) ) {
            cvMul( cvImage, mom.getCvImage(), cvImageTemp );
            swapTemp();
            popROI();       //restore prevoius ROI
            mom.popROI();   //restore prevoius ROI              
            flagImageChanged();
        }
	} else {
        ofLog(OF_ERROR, "in *=, images need to match in size and type");
	}
}

//--------------------------------------------------------------------------------
void ofxCvFloatImage::operator &= ( ofxCvImage& mom ) {
	if( mom.getCvImage()->nChannels == cvImage->nChannels && 
        mom.getCvImage()->depth == cvImage->depth )
    {
        if( pushSetBothToTheirIntersectionROI(*this,mom) ) {
            //this is doing it bit-wise; probably not what we want
            cvAnd( cvImage, mom.getCvImage(), cvImageTemp );
            swapTemp();
            popROI();       //restore prevoius ROI
            mom.popROI();   //restore prevoius ROI              
            flagImageChanged();
        }
	} else {
        ofLog(OF_ERROR, "in &=, images need to match in size and type");
	}
}


//--------------------------------------------------------------------------------
void ofxCvFloatImage::addWeighted( ofxCvGrayscaleImage& mom, float f ) {
	if( pushSetBothToTheirIntersectionROI(*this,mom) ) {
        convertGrayToFloat(mom.getCvImage(), cvImageTemp);
        cvAddWeighted( cvImageTemp, f, cvImage, 1.0f-f,0, cvImage );
        popROI();       //restore prevoius ROI
        mom.popROI();   //restore prevoius ROI           
        flagImageChanged();
    } else {
        ofLog(OF_ERROR, "in addWeighted, images are different sizes");
    }
}



// Get Pixel Data

//--------------------------------------------------------------------------------
unsigned char*  ofxCvFloatImage::getPixels(){
    if(bPixelsDirty) {
    
        if( cvGrayscaleImage == NULL ) {
            cvGrayscaleImage = cvCreateImage( cvSize(cvImage->width,cvImage->height), IPL_DEPTH_8U, 1 );
        }
         
        cvSetImageROI(cvGrayscaleImage, cvRect(roiX,roiY,width,height));  //make sure ROI is in sync
        convertFloatToGray(cvImage, cvGrayscaleImage);   
    
        if(pixels == NULL) {
            // we need pixels, allocate it
            pixels = new unsigned char[width*height];
            pixelsWidth = width;
            pixelsHeight = height;            
        } else if(pixelsWidth != width || pixelsHeight != height) {
            // ROI changed, reallocate pixels for new size
            delete pixels;
            pixels = new unsigned char[width*height];
            pixelsWidth = width;
            pixelsHeight = height;
        }
        
        // copy from ROI to pixels
        for( int i = 0; i < height; i++ ) {
            memcpy( pixels + (i*width),
                    cvGrayscaleImage->imageData + ((i+roiY)*cvGrayscaleImage->widthStep) + roiX,
                    width );
        }
        bPixelsDirty = false;
    }
	return pixels;        
}

//--------------------------------------------------------------------------------
float*  ofxCvFloatImage::getPixelsAsFloats(){
    if(bFloatPixelsDirty) {
        if(floatPixels == NULL) {
            // we need pixels, allocate it
            floatPixels = new float[width*height];
            floatPixelsW = width;
            floatPixelsH = height;            
        } else if(floatPixelsW != width || floatPixelsH != height) {
            // ROI changed, reallocate floatPixels for new size
            delete floatPixels;
            floatPixels = new float[width*height];
            floatPixelsW = width;
            floatPixelsH = height;
        }
        
        // copy from ROI to pixels
        for( int i = 0; i < height; i++ ) {
            memcpy( floatPixels + (i*width),
                    cvImage->imageData + ((i+roiY)*cvImage->widthStep) + roiX*sizeof(float),
                    width*sizeof(float) );
        }
        bFloatPixelsDirty = false;
    }
	return floatPixels;     
}



// Draw Image



// Image Filter Operations

//--------------------------------------------------------------------------------
void ofxCvFloatImage::contrastStretch() {
	double minVal, maxVal;
	cvMinMaxLoc( cvImage, &minVal, &maxVal, NULL, NULL, 0 );
    rangeMap( cvImage, minVal,maxVal, scaleMin,scaleMax );
    flagImageChanged();
}

//--------------------------------------------------------------------------------
void ofxCvFloatImage::convertToRange(float min, float max ){
    rangeMap( cvImage, scaleMin,scaleMax, min,max);
    flagImageChanged();
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
void ofxCvFloatImage::scaleIntoMe( ofxCvImage& mom, int interpolationMethod ){
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
            ofLog(OF_WARNING, "in scaleIntoMe, setting interpolationMethod to CV_INTER_NN");
    		interpolationMethod = CV_INTER_NN;
    	}
        cvResize( mom.getCvImage(), cvImage, interpolationMethod );
        flagImageChanged();

    } else {
        ofLog(OF_ERROR, "in scaleIntoMe, mom image type has to match");
    }
}
