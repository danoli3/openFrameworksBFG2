/*
 * ofMainLoop.cpp
 *
 *  Created on: Oct 25, 2014
 *      Author: arturo
 */

#include <ofMainLoop.h>
#include "ofWindowSettings.h"
#include "ofConstants.h"

//========================================================================
// default windowing
#ifdef TARGET_NODISPLAY
	#include "ofAppNoWindow.h"
#elif defined(TARGET_OF_IOS)
	#include "ofAppiOSWindow.h"
#elif defined(TARGET_ANDROID)
	#include "ofAppAndroidWindow.h"
#elif defined(TARGET_RASPBERRY_PI)
	#include "ofAppEGLWindow.h"
#elif defined(TARGET_EMSCRIPTEN)
	#include "ofxAppEmscriptenWindow.h"
#else
	#include "ofAppGLFWWindow.h"
#endif


ofMainLoop::ofMainLoop()
:bShouldClose(false)
,status(0)
,allowMultiWindow(true)
,windowLoop(NULL)
,pollEvents(NULL)
,escapeQuits(true){

}

ofMainLoop::~ofMainLoop() {
}

shared_ptr<ofAppBaseWindow> ofMainLoop::createWindow(const ofWindowSettings & settings){
#ifdef TARGET_NODISPLAY
	shared_ptr<ofAppNoWindow> window = shared_ptr<ofAppNoWindow>(new ofAppNoWindow());
#else
	#if defined(TARGET_OF_IOS)
	shared_ptr<ofAppiOSWindow> window = shared_ptr<ofAppiOSWindow>(new ofAppiOSWindow());
	#elif defined(TARGET_ANDROID)
	shared_ptr<ofAppAndroidWindow> window = shared_ptr<ofAppAndroidWindow>(new ofAppAndroidWindow());
	#elif defined(TARGET_RASPBERRY_PI)
	shared_ptr<ofAppEGLWindow> window = shared_ptr<ofAppEGLWindow>(new ofAppEGLWindow());
	#elif defined(TARGET_EMSCRIPTEN)
	shared_ptr<ofxAppEmscriptenWindow> window = shared_ptr<ofxAppEmscriptenWindow>(new ofxAppEmscriptenWindow);
	#elif defined(TARGET_OPENGLES)
	shared_ptr<ofAppGLFWWindow> window = shared_ptr<ofAppGLFWWindow>(new ofAppGLFWWindow());
	#else
	shared_ptr<ofAppGLFWWindow> window = shared_ptr<ofAppGLFWWindow>(new ofAppGLFWWindow());
	#endif
#endif
	addWindow(window);
	window->setup(settings);
	ofAddListener(window->events().keyPressed,this,&ofMainLoop::keyPressed);
	return window;
}

void ofMainLoop::run(shared_ptr<ofAppBaseWindow> window, shared_ptr<ofBaseApp> app){
	windowsApps[window] = app;
	if(app){
		ofAddListener(window->events().setup,app.get(),&ofBaseApp::setup,OF_EVENT_ORDER_APP);
		ofAddListener(window->events().update,app.get(),&ofBaseApp::update,OF_EVENT_ORDER_APP);
		ofAddListener(window->events().draw,app.get(),&ofBaseApp::draw,OF_EVENT_ORDER_APP);
		ofAddListener(window->events().exit,app.get(),&ofBaseApp::exit,OF_EVENT_ORDER_APP);
		ofAddListener(window->events().keyPressed,app.get(),&ofBaseApp::keyPressed,OF_EVENT_ORDER_APP);
		ofAddListener(window->events().keyReleased,app.get(),&ofBaseApp::keyReleased,OF_EVENT_ORDER_APP);
		ofAddListener(window->events().mouseMoved,app.get(),&ofBaseApp::mouseMoved,OF_EVENT_ORDER_APP);
		ofAddListener(window->events().mouseDragged,app.get(),&ofBaseApp::mouseDragged,OF_EVENT_ORDER_APP);
		ofAddListener(window->events().mousePressed,app.get(),&ofBaseApp::mousePressed,OF_EVENT_ORDER_APP);
		ofAddListener(window->events().mouseReleased,app.get(),&ofBaseApp::mouseReleased,OF_EVENT_ORDER_APP);
		ofAddListener(window->events().mouseScrolled,app.get(),&ofBaseApp::mouseScrolled,OF_EVENT_ORDER_APP);
		ofAddListener(window->events().windowEntered,app.get(),&ofBaseApp::windowEntry,OF_EVENT_ORDER_APP);
		ofAddListener(window->events().windowResized,app.get(),&ofBaseApp::windowResized,OF_EVENT_ORDER_APP);
		ofAddListener(window->events().messageEvent,app.get(),&ofBaseApp::messageReceived,OF_EVENT_ORDER_APP);
		ofAddListener(window->events().fileDragEvent,app.get(),&ofBaseApp::dragged,OF_EVENT_ORDER_APP);
	}
	currentWindow = window;
    window->events().notifySetup();
}

void ofMainLoop::run(shared_ptr<ofBaseApp> app){
	if(!windowsApps.empty()){
		run(windowsApps.begin()->first,app);
	}
}

int ofMainLoop::loop(){
	if(!windowLoop){
		while(!bShouldClose && !windowsApps.empty()){
			loopOnce();
		}
	}else{
		windowLoop();
	}
	return status;
}

void ofMainLoop::loopOnce(){
	for(auto i = windowsApps.begin();i!=windowsApps.end();i++){
		if(i->first->getWindowShouldClose()){
			i->first->close();
			windowsApps.erase(i);
		}else{
			currentWindow = i->first;
			i->first->update();
			i->first->draw();
		}
	}
	if(pollEvents){
		pollEvents();
	}
	exitEvent.notify(this);
}

shared_ptr<ofAppBaseWindow> ofMainLoop::getCurrentWindow(){
	return currentWindow;
}

shared_ptr<ofBaseApp> ofMainLoop::getCurrentApp(){
	return windowsApps[currentWindow];
}

ofCoreEvents & ofMainLoop::events(){
	return currentWindow->events();
}

void ofMainLoop::shouldClose(int _status){
	for(auto i = windowsApps.begin();i!=windowsApps.end();i++){
		i->first->setWindowShouldClose();
	}
	bShouldClose = true;
	status = _status;
}

void ofMainLoop::setEscapeQuitsLoop(bool quits){
	escapeQuits = quits;
}

void ofMainLoop::keyPressed(ofKeyEventArgs & key){
	if (key.key == OF_KEY_ESC && escapeQuits == true){				// "escape"
		shouldClose(0);
    }
}
