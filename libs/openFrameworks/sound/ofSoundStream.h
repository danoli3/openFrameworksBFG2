#pragma once

#include "ofConstants.h"
#include "ofBaseTypes.h"
#include "ofBaseApp.h"

#ifdef OF_TARGET_IPHONE
	#error we need swappable sound stream api for iphone
#else
	#include "ofRtAudioSoundStream.h"
	#define OF_SOUND_STREAM_TYPE ofRtAudioSoundStream()
#endif 

void ofSoundStreamSetup(int nOutputChannels, int nInputChannels, ofBaseApp * appPtr = NULL);
void ofSoundStreamSetup(int nOutputChannels, int nInputChannels, int sampleRate, int bufferSize, int nBuffers);
void ofSoundStreamSetup(int nOutputChannels, int nInputChannels, ofBaseApp * appPtr, int sampleRate, int bufferSize, int nBuffers);
void ofSoundStreamStop();
void ofSoundStreamStart();
void ofSoundStreamClose();
void ofSoundStreamListDevices();

class ofSoundStream{
	public:
		ofSoundStream();
		~ofSoundStream();
		
		void setSoundStream(ofBaseSoundStream * soundStreamPtr);
		void listDevices();
	
		void setDeviceID(int deviceID);

		bool setupInput(ofBaseSoundInput * soundInputPtr, int numChannels, int sampleRate, int bufferSize, int nBuffers);		
		bool setupOutput(ofBaseSoundOutput * soundOutputPtr, int numChannels, int sampleRate, int bufferSize, int nBuffers);
		
		void start();
		void stop();
		void close();
		
		long unsigned long getTickCount();
		
	protected:
		
		ofBaseSoundStream * soundStream;
};