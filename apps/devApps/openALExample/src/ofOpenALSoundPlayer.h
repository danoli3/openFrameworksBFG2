#pragma once

#include "ofConstants.h"
#include "ofBaseSoundPlayer.h"
#include "ofEvents.h"
#include "ofThread.h"
#include <AL/al.h>
#include <AL/alc.h>
#include "kiss_fft.h"
#include "kiss_fftr.h"
#include <sndfile.h>
#ifdef OF_USING_MPG123
	#include <mpg123.h>
#endif

#define OF_USING_MPG123

//		TO DO :
//		---------------------------
// 		-fft via fmod, as in the last time...
// 		-close fmod if it's up
//		-loadSoundForStreaming(char * fileName);
//		---------------------------

// 		interesting:
//		http://www.compuphase.com/mp3/mp3loops.htm


// ---------------------------------------------------------------------------- SOUND SYSTEM FMOD

// --------------------- global functions:
void ofFmodSoundStopAll();
void ofFmodSoundSetVolume(float vol);
void ofOpenALSoundUpdate();						// calls FMOD update.
float * ofFmodSoundGetSpectrum(int nBands);		// max 512...


// --------------------- player functions:
class ofOpenALSoundPlayer : public ofBaseSoundPlayer, public ofThread {

	public:

		ofOpenALSoundPlayer();
		virtual ~ofOpenALSoundPlayer();

		void loadSound(string fileName, bool stream = false);
		void unloadSound();
		void play();
		void stop();

		void setVolume(float vol);
		void setPan(float vol);
		void setSpeed(float spd);
		void setPaused(bool bP);
		void setLoop(bool bLp);
		void setMultiPlay(bool bMp);
		void setPosition(float pct); // 0 = start, 1 = end;

		float getPosition();
		bool getIsPlaying();
		float getSpeed();
		float getPan();
		bool getIsPaused();

		static void initialize();
		static void close();

		float * getSpectrum(int bands);

		static float * getSystemSpectrum(int bands);

	protected:
		void threadedFunction();

	private:
		friend void ofOpenALSoundUpdate();
		void update(ofEventArgs & args);
		void initFFT(int bands);
		float * getCurrentBufferSum(int size);

		static void createWindow(int size);
		static void runWindow(vector<float> & signal);
		static void initSystemFFT(int bands);

		bool sfReadFile(string path,vector<short> & buffer,vector<float> & fftAuxBuffer);
		bool sfStream(string path,vector<short> & buffer,vector<float> & fftAuxBuffer);
#ifdef OF_USING_MPG123
		bool mpg123ReadFile(string path,vector<short> & buffer,vector<float> & fftAuxBuffer);
		bool mpg123Stream(string path,vector<short> & buffer,vector<float> & fftAuxBuffer);
#endif

		void readFile(string fileName,vector<short> & buffer);
		void stream(string fileName, vector<short> & buffer);

		bool isStreaming;
		bool bMultiPlay;
		bool bLoop;
		bool bLoadedOk;
		bool bPaused;
		float pan; // 0 - 1
		float volume; // 0 - 1
		float internalFreq; // 44100 ?
		float speed; // -n to n, 1 = normal, -1 backwards
		unsigned int length; // in samples;

		static ALCdevice * alDevice;
		static ALCcontext * alContext;
		static vector<float> window;
		static float windowSum;

		int channels;
		float duration; //in secs
		int samplerate;
		vector<ALuint> buffers;
		vector<ALuint> sources;

		// fft structures
		vector<vector<float> > fftBuffers;
		kiss_fftr_cfg fftCfg;
		vector<float> windowedSignal;
		vector<float> bins;
		vector<kiss_fft_cpx> cx_out;


		static kiss_fftr_cfg systemFftCfg;
		static vector<float> systemWindowedSignal;
		static vector<float> systemBins;
		static vector<kiss_fft_cpx> systemCx_out;

		SNDFILE* streamf;
		size_t stream_samples_read;
		mpg123_handle * mp3streamf;
		int stream_encoding;
		int mp3_buffer_size;
		int stream_subformat;
		double stream_scale;
		vector<short> buffer;
		vector<float> fftAuxBuffer;

		bool stream_end;
};

