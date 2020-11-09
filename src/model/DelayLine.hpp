
#define FFT_DELAY_LINE_SIZE 1024
#define DELAY_LINE_SIZE 1<<16

#include "fft/kiss_fft.h"


template <typename T>
struct DelayLine {

    T DelayBuffer[DELAY_LINE_SIZE] = {0.0f};
    int writePtr = 0; // write ptr

    void write(T in) {
        DelayBuffer[writePtr++] = in; 

		if (writePtr >= DELAY_LINE_SIZE) { 
			writePtr -= DELAY_LINE_SIZE; 
		}
    }

	T getNonInterpolatedDelay(int delay) {
		T out;

		int readPtr = writePtr - delay;
		if (readPtr<0) readPtr += DELAY_LINE_SIZE;
		
		out = DelayBuffer[readPtr];
		return out;
	}

	T getDelay(float delay) {
		T out;

		float readPtrf = writePtr - delay;
		readPtrf = std::fmin(readPtrf, writePtr-3.0f);
		int read0 = floor(readPtrf)-1;
		int read1 = read0 + 1;
		int read2 = read0 + 2;
		int read3 = read0 + 3;
		float fDelay = readPtrf - floor(readPtrf) + 1.0f;

		if (read0<0) read0 += DELAY_LINE_SIZE;
		if (read1<0) read1 += DELAY_LINE_SIZE;
		if (read2<0) read2 += DELAY_LINE_SIZE;
		if (read3<0) read3 += DELAY_LINE_SIZE;
	    	
		out =   DelayBuffer[read3] *  fDelay       * (fDelay-1.0f) * (fDelay-2.0f) / 6.0f 
	          - DelayBuffer[read2] *  fDelay       * (fDelay-1.0f) * (fDelay-3.0f) / 2.0f 
        	  + DelayBuffer[read1] *  fDelay       * (fDelay-2.0f) * (fDelay-3.0f) / 2.0f 
        	  - DelayBuffer[read0] * (fDelay-1.0f) * (fDelay-2.0f) * (fDelay-3.0f) / 6.0f;


		return out;
	}

};



struct FFTDelayLine {

    kiss_fft_cpx DelayBuffer[FFT_DELAY_LINE_SIZE] = {{0.0f, 0.0f}};
    int readPtr = 0; // read ptr
	int desiredReadPtr = 0;
    int writePtr = 0; // write ptr
	float balance = 0.0;

	float lerp(float v0, float v1, float t) {
		return (1 - t) * v0 + t * v1;
	}

	kiss_fft_cpx lerp(kiss_fft_cpx v0, kiss_fft_cpx v1, float t) {
		return {(1 - t) * v0.r + t * v1.r,(1 - t) * v0.i + t * v1.i};
	}



	void setDelayTime(int delaySize) {
		readPtr = writePtr - delaySize;
		if (readPtr < 0) { 
			readPtr += FFT_DELAY_LINE_SIZE;
		}
	}

    void write(kiss_fft_cpx in) {
        DelayBuffer[writePtr++] = in; 

		if (writePtr >= FFT_DELAY_LINE_SIZE) { 
			writePtr -= FFT_DELAY_LINE_SIZE; 
		}
    }

	kiss_fft_cpx getValue()
	{
		kiss_fft_cpx out;
				
		out = DelayBuffer[readPtr];

		readPtr++;

		if (readPtr >= FFT_DELAY_LINE_SIZE) {
			readPtr -= FFT_DELAY_LINE_SIZE;
		}

		return out;

	}
};