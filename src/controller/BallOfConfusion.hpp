#include "../FrequencyDomain.hpp"
#include "../model/dsp/FFT.hpp"
#include "../model/dsp/WindowFunction.hpp"
#include "../model/Buffer.hpp"
#include "../model/point3d.hpp"
#include "../model/noise/noise.hpp"

#include <cstdint>
#include <vector>
#include <future>

#include "osdialog.h"
#include "cmath"
#include <dirent.h>
#include <algorithm> //----added by Joakim Lindbom

//using namespace std;

using namespace frequencydomain::dsp;
using rack::simd::float_4;

#define WAV_TABLE_SIZE 2048
#define MAX_MORPHED_WAVETABLES 4
#define MAX_SAMPLES 16
#define MAX_LIVE_INPUTS 16
#define MAX_VOICE_COUNT 32
#define MAX_POLYPHONY 16
#define MAX_BUFFER_SIZE 32768 //2^15
#define MORPH_MODES 3

template <int OVERSAMPLE, int QUALITY, typename T>
struct VoltageControlledOscillator {
	bool soft = false;
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
		// if (soft) {
		// 	// Reverse direction
		// 	deltaPhase *= syncDirection;
		// }
		// else {
		// 	// Reset back to forward
		// 	syncDirection = 1.f;
		// }
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
                    T newPhase = simd::ifelse(phase >= softSyncPhase, basePhase, phase);					
                    phase = newPhase;                    
					//syncDirection = simd::ifelse(sync, -syncDirection, syncDirection);
				}
				// else {
					// T newPhase = simd::ifelse(sync, (1.f - syncCrossing) * deltaPhase, phase);
					// // Insert minBLEP for sync
					// for (int i = 0; i < channels; i++) {
					// 	if (syncMask & (1 << i)) {
					// 		T mask = simd::movemaskInverse<T>(1 << i);
					// 		float p = syncCrossing[i] - 1.f;
					// 		T x;
					// 		x = mask & (sqr(newPhase) - sqr(phase));
					// 		sqrMinBlep.insertDiscontinuity(p, x);
					// 		x = mask & (saw(newPhase) - saw(phase));
					// 		sawMinBlep.insertDiscontinuity(p, x);
					// 		x = mask & (tri(newPhase) - tri(phase));
					// 		triMinBlep.insertDiscontinuity(p, x);
					// 		x = mask & (sin(newPhase) - sin(phase));
					// 		sinMinBlep.insertDiscontinuity(p, x);
					// 	}
					// }
                    phase = basePhase;
				// }
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
    // uint16_t wti() {
	// 	return uint16_t(oscValue);
	// }


};



struct BallOfConfusionModule : Module {
    enum ParamIds {
        YAW_PARAM,
        PITCH_PARAM,
        ROLL_PARAM,
        QUANTIZE_YAW_PARAM,
        QUANTIZE_PITCH_PARAM,
        QUANTIZE_ROLL_PARAM,
        FM_AMOUNT,
        FREQUENCY_PARAM,
        SYNC_MODE_PARAM,
        MORPH_MODE_PARAM,
        SYNC_POSITION_PARAM,
        SPECTRUM_SHIFT_PARAM,
        NUM_PARAMS
    };

    enum InputIds {
        YAW_INPUT,
        PITCH_INPUT,
        ROLL_INPUT,
        V_OCTAVE_INPUT,
        FM_INPUT,
        FM_AMOUNT_INPUT,
        PHASE_INPUT,
        SYNC_INPUT,
        SYNC_POSITION_INPUT,
        SPECTRUM_SHIFT_INPUT,
        NUM_INPUTS
    };
    enum OutputIds { OUTPUT_L, OUTPUT_R, DEBUG_OUTPUT, NUM_OUTPUTS };
    enum LightIds {
        SYNC_MODE_LIGHT,
        MORPH_MODE_LIGHT = SYNC_MODE_LIGHT + 3,
        YAW_QUANTIZED_LIGHT = MORPH_MODE_LIGHT + 3,
        PITCH_QUANTIZED_LIGHT = YAW_QUANTIZED_LIGHT + 3,
        ROLL_QUANTIZED_LIGHT = PITCH_QUANTIZED_LIGHT + 3,
        NUM_LIGHTS = ROLL_QUANTIZED_LIGHT + 3
    };

    BallOfConfusionModule ();
    ~BallOfConfusionModule ();

    void clearSamples();
    void loadDirectory(std::string path);
    void loadIndividualWavefile(std::string path);
    void loadAdditionalWavefile(std::string path);
    void loadSample(uint8_t slot, std::string path);
    void fibonacci_sphere(uint16_t samples);
    void rotateSphere(float yaw,float pitch,float roll);
    void buildActualWaveTable();
    void process (const ProcessArgs &args) override;
    float paramValue (uint16_t, uint16_t, float, float);


    void onReset() override;
    void dataFromJson(json_t *) override;
    json_t *dataToJson() override;



    std::vector<std::vector<float>> playBuffer[MAX_SAMPLES];
    std::vector<point3d> sphere;
    std::vector<uint16_t> waveTableFileSampleCount;
    std::vector<float> waveTableList;
    std::vector<float> waveTableSpectralList;
    std::vector<std::string> waveTableNames;

    uint16_t waveTablePathCount = 0;
    uint16_t waveTableFileCount = 0;
    uint16_t waveTableCount = 0;
    uint16_t waveTablesInUse[4] = {0};
    float waveTableDistance[4] = {0};
    float waveTableWeighting[4] = {0};

    float actualWaveTable[WAV_TABLE_SIZE] = {0};

    float fftBuffer[WAV_TABLE_SIZE] = {0}; 

    std::string morphModes[MORPH_MODES] ={"Interpolate","Spectral","Spectral 0"};
    std::string syncModes[2] ={"Hard","Soft"};


    bool loading = false;
    std::vector<std::string> lastPath;
    std::string fileDesc = "<Empty>";
    bool fileLoaded = false;

    int sampnumber = 0; 
    int retard = 0;
    bool reload = false ;
    std::vector<bool> loadFromDirectory;
    std::vector <std::string> fichier;    

    uint32_t samplesPlayed;

    WindowFunction<float> *windowFunction;
    FFT *fft;

    GaussianNoiseGenerator _gauss;
    dsp::SchmittTrigger morphModeTrigger,syncModeTrigger;
    //dsp::PulseGenerator endOfSamplePulse;

	VoltageControlledOscillator<16, 16, float_4> oscillators[4];

    bool rebuild = false;

    float yaw=0;
    float pitch=0;
    float roll=0;
    float lastYaw = -1;
    float lastPitch = -1;
    float lastRoll = -1;

    float scatterPercent = 0;
    float lastScatterPercent = 0;

    int morphMode = 0;
    int lastMorphMode = -1;

    float phase=0;
    float currentWavePosition = 0;

    bool syncMode = false; // true = soft
    float syncPosition = 1;

    uint8_t windowFunctionId = 4;

    int16_t spectrumShift = 0;
    int16_t lastSpectrumShift = 0;

    float sampleRate;
    // uint64_t sampleCounter = 0;
    // uint64_t lastGrainSampleCount = 0;

    float windowFunctionSize = 0; 
    // uint8_t windowFunctionId;
    // uint8_t lastWindowFunctionId;




    // percentages
    float yawPercentage = 0;
    float pitchPercentage = 0;
    float rollPercentage = 0;

    float tuningPercentage = 0;
    float fmAmountPercentage = 0;
    float syncPositionPercentage = 0;
    float spectrumShiftPercentage = 0;


    // //Grain Vectors
    // std::vector<std::vector<float>> grains;
    // std::vector<uint8_t> individualGrainVoice;
    // std::vector<float> individualGrainPosition;
    // std::vector<float> individualGrainPitch;
    // std::vector<float> individualGrainPanning;
    // std::vector<uint8_t> individualGrainWindowFunction;
    // std::vector<uint64_t> individualGrainSpawnTime;
    // std::vector<bool> individualGrainReversed;

};
