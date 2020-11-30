
/** Based on "The Voss algorithm"
http://www.firstpr.com.au/dsp/pink-noise/
*/
template <int QUALITY = 8>
struct PinkNoiseGenerator {
	int frame = -1;
	float values[QUALITY] = {};

	float process() {
		int lastFrame = frame;
		frame++;
		if (frame >= (1 << QUALITY))
			frame = 0;
		int diff = lastFrame ^ frame;

		float sum = 0.f;
		for (int i = 0; i < QUALITY; i++) {
			if (diff & (1 << i)) {
				values[i] = random::uniform() - 0.5f;
			}
			sum += values[i];
		}
		return sum;
	}
};


struct InverseAWeightingFFTFilter {
	static constexpr int BUFFER_LEN = 1024;

	alignas(16) float inputBuffer[BUFFER_LEN] = {};
	alignas(16) float outputBuffer[BUFFER_LEN] = {};
	int frame = 0;
	dsp::RealFFT fft;

	InverseAWeightingFFTFilter() : fft(BUFFER_LEN) {}

	float process(float deltaTime, float x) {
		inputBuffer[frame] = x;
		if (++frame >= BUFFER_LEN) {
			frame = 0;
			alignas(16) float freqBuffer[BUFFER_LEN * 2];
			fft.rfft(inputBuffer, freqBuffer);

			for (int i = 0; i < BUFFER_LEN; i++) {
				float f = 1 / deltaTime / 2 / BUFFER_LEN * i;
				float amp = 0.f;
				if (80.f <= f && f <= 20000.f) {
					float f2 = f * f;
					// Inverse A-weighted curve
					amp = ((424.36f + f2) * std::sqrt((11599.3f + f2) * (544496.f + f2)) * (148693636.f + f2)) / (148693636.f * f2 * f2);
				}
				freqBuffer[2 * i + 0] *= amp / BUFFER_LEN;
				freqBuffer[2 * i + 1] *= amp / BUFFER_LEN;
			}

			fft.irfft(freqBuffer, outputBuffer);
		}
		return outputBuffer[frame];
	}
};


enum NoiseColor {
    NOISE_WHITE,   
    NOISE_PINK, // Pink noise: -3dB/oct
    NOISE_RED,    // apply nonlinearities to feedback paths
    NOISE_VIOLET,     // apply nonlinearities to both states and feedback paths
	NOISE_GREY,
	NOISE_BLUE,
	NOISE_BLACK,
//	NOISE_GAUSSIAN
};




struct NoiseGenerator  {

	dsp::ClockDivider blackDivider;
	PinkNoiseGenerator<8> pinkNoiseGenerator;
	dsp::IIRFilter<2, 2> redFilter;
	float lastWhite = 0.f;
	float lastPink = 0.f;
    float sampleTime;
	InverseAWeightingFFTFilter greyFilter;

	// For calibrating levels
	// float meanSqr = 0.f;

	NoiseGenerator() {
	
		// Hard-code coefficients for Butterworth lowpass with cutoff 20 Hz @ 44.1kHz.
		const float b[] = {0.00425611, 0.00425611};
		const float a[] = {-0.99148778};
		redFilter.setCoefficients(b, a);
	}


	float getNoise(NoiseColor noiseColor) {
		// White noise: equal power density
		const float gain = 5.f / std::sqrt(2.f);

		float white = random::normal();
        float pink = pinkNoiseGenerator.process() / 0.816f;
        float violet, blue, u;

		float noiseOutput;
		switch(noiseColor) {
			case NOISE_WHITE: // White noise: equal power density
				noiseOutput = white * gain;
				break;
			case NOISE_RED: // Red/Brownian noise: -6dB/oct
				noiseOutput = redFilter.process(white) / 0.06465f * gain;
				break;
			case NOISE_VIOLET: // Violet/purple noise: 6dB/oct
                violet = (white - lastWhite) / 1.41f;
				lastWhite = white;
				noiseOutput = violet * gain;
				break;
			case NOISE_GREY: // Gray noise: psychoacoustic equal loudness curve, specifically inverted A-weighted
				noiseOutput = greyFilter.process(sampleTime, white) / 1.67f * gain;
				break;
			case NOISE_PINK: // Pink noise: -3dB/oct
				noiseOutput = pink * gain;
				break;
			case NOISE_BLUE: // Blue noise: 3dB/oct
				blue = (pink - lastPink) / 0.705f;
				lastPink = pink;
                noiseOutput = blue * gain;
				break;
            case NOISE_BLACK: // Black noise: uniform noise
		                      // Note: I (Andrew Belt) made this definition up.
                u = random::uniform();
			    noiseOutput = u * 10.f - 5.f;
                break;

		}		

        return noiseOutput;
	}





};

