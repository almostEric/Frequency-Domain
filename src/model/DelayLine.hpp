
#define DELAY_LINE_SIZE 1024

#include "fft/kiss_fft.h"

struct DelayLine {

    kiss_fft_cpx DelayBuffer[DELAY_LINE_SIZE] = {{0.0f, 0.0f}};
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
			readPtr += DELAY_LINE_SIZE;
		}
	}

    void write(kiss_fft_cpx in) {
        DelayBuffer[writePtr++] = in; 

		if (writePtr >= DELAY_LINE_SIZE) { 
			writePtr -= DELAY_LINE_SIZE; 
		}
    }

	kiss_fft_cpx getValue()
	{
		kiss_fft_cpx out;
				
		out = DelayBuffer[readPtr];

		readPtr++;

		if (readPtr >= DELAY_LINE_SIZE) {
			readPtr -= DELAY_LINE_SIZE;
		}

		return out;

	}
};