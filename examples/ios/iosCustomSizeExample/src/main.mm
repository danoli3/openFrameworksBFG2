#include "ofMain.h"
#include "ofApp.h"

int main(){

    bool bUseNative = true;
    
    if (bUseNative){
        /**
         *
         *  Below is how you start using a native ios setup.
         *
         *  First a ofAppiOSWindow is created and added to ofSetupOpenGL()
         *  Notice that no app is being sent to ofRunApp() - this happens later when we actually need the app.
         *
         *  One last thing that needs to be done is telling ofAppiOSWindow which AppDelegate to use.
         *  This is a custom AppDelegate and inside it you can start coding your native iOS application.
         *  The AppDelegate must extend ofxiOSAppDelegate.
         *
         **/
        
        ofAppiOSWindow *window = new ofAppiOSWindow();
        ofSetupOpenGL(ofPtr<ofAppBaseWindow>(window), 1024,768, OF_FULLSCREEN);
        window->startAppWithDelegate("MyAppDelegate");
    }
    else {
        /**
         *
         *  This is the normal way of running an app using ofxiOS.
         *  This code has been left in this example to show that ofxiOS still works
         *
         **/
        
        ofSetupOpenGL(1024,768, OF_FULLSCREEN);
        ofRunApp(new ofApp());
    }
}
