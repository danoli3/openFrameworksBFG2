
#ifndef OFX_CV_COLOR_IMAGE_H
#define OFX_CV_COLOR_IMAGE_H

#include "ofxCvImage.h"


class ofxCvColorImage : public ofxCvImage {


  public:

    ofxCvColorImage() {};
    ofxCvColorImage( const ofxCvColorImage& mom );
    void allocate( int w, int h );


    // Set Pixel Data
    //
    void set( float value );
    void set( int valueR, int valueG, int valueB);
    virtual void operator -= ( float value );
    virtual void operator += ( float value ); 
        
    void setFromPixels( unsigned char * _pixels, int w, int h );
    void setFromGrayscalePlanarImages( const ofxCvGrayscaleImage& red, const ofxCvGrayscaleImage& green, const ofxCvGrayscaleImage& blue );

    void operator = ( unsigned char* _pixels );
    void operator = ( const ofxCvGrayscaleImage& mom );
    void operator = ( const ofxCvColorImage& mom );
    void operator = ( const ofxCvFloatImage& mom );   


    // Get Pixel Data
    //
    unsigned char*  getPixels();
    void convertToGrayscalePlanarImages( ofxCvGrayscaleImage& red, ofxCvGrayscaleImage& green, ofxCvGrayscaleImage& blue );
    

    // Draw Image
    //
    void draw( float x, float y );
    void draw( float x, float y, float w, float h );


    // Image Transformation Operations
    //
    void resize( int w, int h );
    void scaleIntoMe( const ofxCvImage& mom, int interpolationMethod = CV_INTER_NN);
    void convertRgbToHsv();
    void convertHsvToRgb();    

};



#endif
