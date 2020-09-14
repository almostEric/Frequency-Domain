#pragma once

using simd::float_4;

enum OscillatorWaveShapes {
    SIN_WAVESHAPE,
    TRIANGLE_WAVESHAPE,
    SAWTOOTH_WAVESHAPE,
    SQUARE_WAVESHAPE,
    RECTANGLE_WAVESHAPE,
};

template <typename T>
struct Oscillator {
  Oscillator() {
    reset();
  }

  void reset() {
    frequency = 0;
    delta = 0.5;
    pw50 = 0.5;
    pw25 = 0.25;
  }

  void setFrequency(T frequency) {
    this->frequency = frequency;
  }

  void setPhase(T phase) {
    this->delta = phase;
  }

  void setPw(T pw) {
    this->pw = pw;
  }

  void resync() {
    this->delta = 0.5;
  }

  void setDelta(T delta) {
    this->delta = delta;
  }

  void step(T sampleTime) {
    T frequencyTime = frequency * sampleTime;

    delta += frequencyTime;
    if (delta[0] >= 1.0f) {
      delta[0] -= 1.0f;
    }
    if (delta[1] >= 1.0f) {
      delta[1] -= 1.0f;
    }
    if (delta[2] >= 1.0f) {
      delta[2] -= 1.0f;
    }
    if (delta[3] >= 1.0f) {
      delta[3] -= 1.0f;
    }
  }

  T saw() {
    return (delta * 2) - 1;
  }

  T tri() {
    return simd::ifelse(delta > 0.5, 2.0 * delta, 2.0 * (1.0 - delta)) - 1;
  }

  T sin() {
    return -simd::sin(2.0 * M_PI * delta);
  }

  T sqr() {
    return simd::ifelse(delta < pw50, 1.0, -1.0);
  }

  T rect() {
    return simd::ifelse(delta < pw25, 1.0, -1.0);
  }

  // percentage through the current waveform
  T frequency;
  
  T delta;
  T pw50;
  T pw25;
};


template <int OVERSAMPLE, int QUALITY, int WAV_TABLE_SIZE, typename T>
struct WavelessOscillator {
	bool soft = false;
	bool softReverse = false;
	bool syncEnabled = false;
	// For optimizing in serial code
	int channels = 0;

	T lastSyncValue = 0.f;
	T phase = 0.f;
    T softSyncPhase = 1.0f;
	T freq;
    T basePhase = 0.f;
	T syncDirection = 1.f;

	T oscValue = 0.f;

	void setPitch(T pitch) {
		freq = dsp::FREQ_C4 * dsp::approxExp2_taylor5(pitch + 30) / 1073741824;
	}

    void setBasePhase(T initialPhase) {
        //Apply change, then remember
        phase += initialPhase - basePhase;
        phase -= simd::floor(phase);
        basePhase = initialPhase;
    }

    void setSoftSyncPhase(T ssPhase) {
        //Apply change, then remember
        softSyncPhase = ssPhase;
    }

	
	void process(float deltaTime, T syncValue) {
		// Advance phase
		T deltaPhase = simd::clamp(freq * deltaTime, 1e-6f, 0.35f);
		if (softReverse) {
			// Reverse direction
			deltaPhase *= syncDirection;
		}
		else {
			// Reset back to forward
			syncDirection = 1.f;
		}
		phase += deltaPhase;
		// Wrap phase
		phase -= simd::floor(phase);


        // Detect sync
		// Might be NAN or outside of [0, 1) range
		if (syncEnabled) {
			T deltaSync = syncValue - lastSyncValue;
			T syncCrossing = -lastSyncValue / deltaSync;
			lastSyncValue = syncValue;
			T sync = (0.f < syncCrossing) & (syncCrossing <= 1.f) & (syncValue >= 0.f);
			int syncMask = simd::movemask(sync);
			if (syncMask) {
				if (soft) {
            if(softReverse) {
                syncDirection = simd::ifelse(phase >= softSyncPhase, syncDirection, -syncDirection);
            } else {            
              T newPhase = simd::ifelse(phase >= softSyncPhase, phase, basePhase);					
              phase = newPhase;                    
            }
				} else {
          phase = basePhase;
        }
			}
		}

		// Table Ubdex
		oscValue = wt(phase);
		
		}

	T wt(T phase) {
		return phase * WAV_TABLE_SIZE;
	}
	T wt() {
		return oscValue;
	}


};

