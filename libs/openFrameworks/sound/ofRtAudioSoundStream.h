#pragma once

#include "ofConstants.h"
#include "ofBaseSoundStream.h"
#include "RtAudio.h"

class RtAudio;

class ofRtAudioSoundStream : public ofBaseSoundStream{
	public:
		ofRtAudioSoundStream();
		~ofRtAudioSoundStream();
		
		void listDevices();
		void setDeviceID(int deviceID);

		void setInput(ofBaseSoundInput * soundInput);
		void setOutput(ofBaseSoundOutput * soundOutput);
		bool setup(int outChannels, int inChannels, int sampleRate, int bufferSize, int nBuffers);
		bool setup(ofBaseApp * app, int outChannels, int inChannels, int sampleRate, int bufferSize, int nBuffers);
		
		void start();
		void stop();
		void close();
		
		long unsigned long getTickCount();		
				
		int getNumInputChannels();
		int getNumOutputChannels();
	
		
	private:
		long unsigned long	tickCount;
		RtAudio *			audio;
		int					sampleRate;
		int					deviceID;
		int					nInputChannels;
		int					nOutputChannels;
		ofBaseSoundInput *  soundInputPtr;
		ofBaseSoundOutput * soundOutputPtr;
		
		static int rtAudioCallback(void *outputBuffer, void *inputBuffer, unsigned int bufferSize, double streamTime, RtAudioStreamStatus status, void *data);

};


