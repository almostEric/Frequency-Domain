

// "amplitude" is 0-whatever here, with 1 (=0db) meaning unity gain.
inline float decibelsToAmplitude(float db) {
	return powf(10.0f, db * 0.05f);
}

inline float amplitudeToDecibels(float amplitude) {
	return 20.0f * log10f(amplitude);
}
