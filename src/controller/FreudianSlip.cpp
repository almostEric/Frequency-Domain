#include "FreudianSlip.hpp"
#include <cmath>
#define DR_WAV_IMPLEMENTATION
#include "../model/dr_wav.h"
#include "../model/Interpolate.hpp"
#include "../component/tooltip.hpp"



FreudianSlipModule::FreudianSlipModule() {
    config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);


    configParam(VOICE_COUNT, 1.0f, 32.0f, 32.0f,"# of Voices");
    // this is SOOOOO hacky
    configParam<VCOVoice>(VOICE_WAVEFORM, 0.0f, 4.0f, 0.0f,"Voice Waveform");
    configParam(V_OCT_PARAM, -3.0f, 3.0f, 0.0f, "Tuning");
    configParam(FREQ_WARP_AMOUNT, 0.0f, 1.0f, 0.0f,"Frequency Warp Amount","%",0,100);
    configParam(FREQ_WARP_CENTER, 0.0f, 1.0f, 0.0f,"Frequency Warp Center Frequency", " Hz", 0, 10000);
    configParam(RANDOMIZE_PARAM, 0.0f, 1.0f, 0.0f, "Randomize","%",0,100);

    configParam(FM_AMOUNT, 0.0f, 1.0f, 0.0f, "FM Amount","%",0,100);
    configParam(RM_MIX, 0.0f, 1.0f, 0.0f, "AM/RM Mix","%",0,100);
    configParam(PLAY_SPEED_PARAM, -3.0f, 3.0f, 1.0f, "Play Speed","%",0,100);
    configParam(START_POS_PARAM, 0.0f, 1.0f, 0.0f, "Start Position","%",0,100);
    configParam(STOP_POS_PARAM, -1.0f, 0.0f, 0.0f, "Stop Position","%",0,100);

    configParam(RING_MODULATION, 0.0f, 1.0f, 0.0f, "Ring Modulation");

    playBuffer.resize(2);
    playBuffer[0].resize(0);
    playBuffer[1].resize(0); 

    voiceAnalysis1.resize(0);
    voiceAnalysis2.resize(0);



    // frameSize / sampleRate
    frameSize = 12;
    lastFrameSize = frameSize;
    hopSize = frameSize - 7;  
    frameSizeInBytes = pow(2.0f, frameSize);
    hopSizeInBytes = pow(2.0f, hopSize);


    sampleRate = APP->engine->getSampleRate();

    // window function
    windowFunctionId = 7;
    lastWindowFunctionId = 7;


    fft1 = new FFT(frameSizeInBytes);
    fft2 = new FFT(frameSizeInBytes);

    binnings1 = new Binning(frameSizeInBytes, sampleRate);
    binnings2 = new Binning(frameSizeInBytes, sampleRate);


    windowFunction = new WindowFunction<float>(frameSizeInBytes);

    for (uint16_t i = 0; i < MAX_FRAMES; i++) {
      dryBuffer1[i] = new Buffer<float>(frameSizeInBytes, hopSizeInBytes * i);
      dryBuffer2[i] = new Buffer<float>(frameSizeInBytes, hopSizeInBytes * i);
    }

    // set up one storage for matrix displays
    frequencyModulationCells = new OneDimensionalCellsWithRollover(32, 32, 0 , 1, PIN_ROLLOVER_MODE, WRAP_AROUND_ROLLOVER_MODE);
    frequencyModulationAmountCells = new OneDimensionalCellsWithRollover(32, 32, 0, 1,PIN_ROLLOVER_MODE, WRAP_AROUND_ROLLOVER_MODE);
    ringModulationCells = new OneDimensionalCellsWithRollover(32, 32, 0, 1, PIN_ROLLOVER_MODE, WRAP_AROUND_ROLLOVER_MODE);
    ringModulationMixCells = new OneDimensionalCellsWithRollover(32, 32, 0, 1,PIN_ROLLOVER_MODE, WRAP_AROUND_ROLLOVER_MODE);
    panningCells = new OneDimensionalCellsWithRollover(32, 32, -1, 1, PIN_ROLLOVER_MODE, WRAP_AROUND_ROLLOVER_MODE);
    playSpeedCells = new OneDimensionalCellsWithRollover(32, 32, -3, 3, PIN_ROLLOVER_MODE, WRAP_AROUND_ROLLOVER_MODE);
    startPositionCells = new OneDimensionalCellsWithRollover(32, 32, 0, 1,PIN_ROLLOVER_MODE, WRAP_AROUND_ROLLOVER_MODE);
    stopPositionCells = new OneDimensionalCellsWithRollover(32, 32, 0, 1,PIN_ROLLOVER_MODE, WRAP_AROUND_ROLLOVER_MODE,1.0);


    srand(time(NULL));

    onReset();
}

FreudianSlipModule::~FreudianSlipModule() {
  for (uint16_t i = 0; i < MAX_FRAMES; i++) {
    if (dryBuffer1[i]) {
      delete dryBuffer1[i];
    }
    if (dryBuffer2[i]) {
      delete dryBuffer2[i];
    }
  }


  delete windowFunction;
  
  delete fft1;
  delete fft2;

  delete binnings1;
  delete binnings2;

  delete frequencyModulationCells;
  delete frequencyModulationAmountCells;
  delete ringModulationCells;
  delete ringModulationMixCells;
  delete panningCells;
  delete playSpeedCells;
  delete startPositionCells;
  delete stopPositionCells;

}

void FreudianSlipModule::onReset() {
    // window function
    windowFunctionId = 7;

    // reset the cells
    frequencyModulationCells->reset();
    frequencyModulationAmountCells->reset();
    ringModulationCells->reset();
    ringModulationMixCells->reset();
    panningCells->reset();
    playSpeedCells->reset();
    startPositionCells->reset();
    stopPositionCells->reset();

    loopMode = false;
    positionMode = false;
    eocMode= false;

}

void FreudianSlipModule::dataFromJson(json_t *root) {

    // lastPath
    json_t *lastPathJ = json_object_get(root, "lastPath");
    if (lastPathJ) {
        lastPath = json_string_value(lastPathJ);
        reload = true ;
        loadSample(lastPath);
    }

    // json_t *fs = json_object_get(root, "frameSize");
    // if (json_is_integer(fs)) {
    // frameSize = (uint8_t) json_integer_value(fs);
    // }
    json_t *win = json_object_get(root, "windowFunction");
    if (json_is_integer(win)) {
        windowFunctionId = (uint8_t) json_integer_value(win);
    }
    json_t *rm = json_object_get(root, "rmActive");
    if (json_is_boolean(rm)) {
        rmActive = json_boolean_value(rm);
    }
    json_t *wf = json_object_get(root, "warpBasedOnFundamental");
    if (json_is_boolean(wf)) {
        warpBasedOnFundamental = json_boolean_value(wf);
    }
    json_t *lm = json_object_get(root, "loopMode");
    if (json_is_boolean(lm)) {
        loopMode = json_boolean_value(lm);
    }
    json_t *pm = json_object_get(root, "positionMode");
    if (json_is_boolean(pm)) {
        positionMode = json_boolean_value(pm);
    }
    json_t *em = json_object_get(root, "eocMode");
    if (json_is_boolean(em)) {
        eocMode = json_boolean_value(em);
    }

    for(int i=0;i<MAX_VOICE_COUNT;i++) {
        std::string buf = "fmMatrix-" + std::to_string(i) ;
        json_t *fmmJ = json_object_get(root, buf.c_str());
        if (fmmJ) {
            frequencyModulationCells->cells[i] = json_real_value(fmmJ);
        }
        buf = "fmAmount-" + std::to_string(i) ;
        json_t *fmaJ = json_object_get(root, buf.c_str());
        if (fmaJ) {
            frequencyModulationAmountCells->cells[i] = json_real_value(fmaJ);
        }
        buf = "rmMatrix-" + std::to_string(i) ;
        json_t *rmJ = json_object_get(root, buf.c_str());
        if (rmJ) {
            ringModulationCells->cells[i] = json_real_value(rmJ);
        }
        buf = "rmMix-" + std::to_string(i) ;
        json_t *rmmJ = json_object_get(root, buf.c_str());
        if (rmmJ) {
            ringModulationMixCells->cells[i] = json_real_value(rmmJ);
        }
        buf = "panning-" + std::to_string(i) ;
        json_t *panJ = json_object_get(root, buf.c_str());
        if (panJ) {
            panningCells->cells[i] = json_real_value(panJ);
        }
        buf = "playSpeed-" + std::to_string(i) ;
        json_t *playSpeedJ = json_object_get(root, buf.c_str());
        if (playSpeedJ) {
            playSpeedCells->cells[i] = json_real_value(playSpeedJ);
        }
        buf = "startPosition-" + std::to_string(i) ;
        json_t *startPositionJ = json_object_get(root, buf.c_str());
        if (startPositionJ) {
            startPositionCells->cells[i] = json_real_value(startPositionJ);
        }
        buf = "stopPosition-" + std::to_string(i) ;
        json_t *stopPositionJ = json_object_get(root, buf.c_str());
        if (stopPositionJ) {
            stopPositionCells->cells[i] = json_real_value(stopPositionJ);
        }
    }
}

json_t *FreudianSlipModule::dataToJson() {
  json_t *root = json_object();
    json_object_set_new(root, "lastPath", json_string(lastPath.c_str()));	
    //json_object_set(root, "frameSize", json_integer(frameSize));
    json_object_set(root, "windowFunction", json_integer(windowFunctionId));
    json_object_set(root, "rmActive", json_boolean(rmActive));
    json_object_set(root, "warpBasedOnFundamental", json_boolean(warpBasedOnFundamental));
    json_object_set(root, "loopMode", json_boolean(loopMode));
    json_object_set(root, "positionMode", json_boolean(positionMode));
    json_object_set(root, "eocMode", json_boolean(eocMode));
    for(int i=0;i<MAX_VOICE_COUNT;i++) {
        std::string buf = "fmMatrix-" + std::to_string(i) ;
        json_object_set(root, buf.c_str(),json_real((float) frequencyModulationCells->cells[i]));
        buf = "fmAmount-" + std::to_string(i) ;
        json_object_set(root, buf.c_str(),json_real((float) frequencyModulationAmountCells->cells[i]));
        buf = "rmMatrix-" + std::to_string(i) ;
        json_object_set(root, buf.c_str(),json_real((float) ringModulationCells->cells[i]));
        buf = "rmMix-" + std::to_string(i) ;
        json_object_set(root, buf.c_str(),json_real((float) ringModulationMixCells->cells[i]));
        buf = "panning-" + std::to_string(i) ;
        json_object_set(root, buf.c_str(),json_real((float) panningCells->cells[i]));
        buf = "playSpeed-" + std::to_string(i) ;
        json_object_set(root, buf.c_str(),json_real((float) playSpeedCells->cells[i]));
        buf = "startPosition-" + std::to_string(i) ;
        json_object_set(root, buf.c_str(),json_real((float) startPositionCells->cells[i]));
        buf = "stopPosition-" + std::to_string(i) ;
        json_object_set(root, buf.c_str(),json_real((float) stopPositionCells->cells[i]));
    }

    return root;
}

float FreudianSlipModule::paramValue (uint16_t param, uint16_t input, float low, float high) {
  float current = params[param].getValue();

  if (inputs[input].isConnected()) {
    // high - low, divided by one tenth input voltage, plus the current value
    current += ((inputs[input].getVoltage() / 10) * (high - low));
  }

  return clamp(current, low, high);
}



void FreudianSlipModule::loadSample(std::string path) {

		loading = true;
        fileDesc = "Loading...";
		unsigned int c;
  		unsigned int sr;
  		drwav_uint64 tsc;
		float* pSampleData;
		pSampleData = drwav_open_and_read_file_f32(path.c_str(), &c, &sr, &tsc);

	if (pSampleData != NULL) {
		channels = c;
		playerSampleRate = sr;
		playBuffer[0].clear();
		playBuffer[1].clear();
		for (unsigned int i=0; i < tsc; i = i + c) {
			playBuffer[0].push_back(pSampleData[i]);
			if (channels == 2)
				playBuffer[1].push_back((float)pSampleData[i+1]);
			
		}
		totalSampleC = playBuffer[0].size();
		drwav_free(pSampleData);
        loading = false;


		fileLoaded = true;
		fileDesc = system::getFilename(path).substr(0,80) + "\n";
		fileDesc += std::to_string(channels)+ " channel" + (channels > 1 ? "s" : "") + ", ";
		fileDesc += std::to_string(playerSampleRate)+ " Hz" + "\n";

        //fprintf(stderr, "%s \n", fileDesc.c_str());
		
        lastPath = path;
        analysisStatus = 0;
	}
	else {
		
		fileLoaded = false;
	}
};


void FreudianSlipModule::analyze(){
    fprintf(stderr, "Starting Analysis \n");
    analysisStatus = 1; //1 is in progress
    voiceAnalysis1.clear();
    voiceAnalysis2.clear();
    uint32_t uiFrameIndex = 0;
    uint32_t uiFrameStepSize = totalSampleC / (hopSizeInBytes * NUM_UI_FRAMES) + 1;
    sampleStatusDesc = "Analyzing...";

    for (uint16_t i = 0; i < MAX_FRAMES; i++) {
      dryBuffer1[i]->reset();
      dryBuffer2[i]->reset();
    }


    int lastUiBand = -1;
    int uiBand = 0;
    int uiBandCount = 0;
    float uiBandMagnitude = 0.0;
    for (int i=0; i < floor(totalSampleC); i++) { //neeD TO PAD WITH 0s to fill out last frame
        for(uint16_t f=0; f < MAX_FRAMES; f++) {
            float windowedValue = windowFunction->windowValue(windowFunctionId, dryBuffer1[f]->setPos);
            dryBuffer1[f]->set(playBuffer[0][i] * windowedValue);
            if(channels > 1)
                dryBuffer2[f]->set(playBuffer[1][i] * windowedValue);

            if (dryBuffer1[f]->looped) {
                //updateCells = true;
                fft1->fft(dryBuffer1[f]->data);
                if(channels > 1)
                    fft2->fft(dryBuffer2[f]->data);
                for(uint16_t bandIndex=0;bandIndex<=frameSizeInBytes / 2;bandIndex++) {

                    kiss_fft_cpx outValue1 = fft1->out[bandIndex]; 

                    //store magnitude
                    float magnitude1 = sqrt(outValue1.r * outValue1.r + outValue1.i * outValue1.i);
                    freqDomain1[bandIndex] = magnitude1;

                    if(uiFrameIndex % uiFrameStepSize == 0 && bandIndex > 0) {
                      uiBand = (log10(float(bandIndex) * sampleRate / frameSizeInBytes ) - 1.0) * 19.142; //fitting 1000 bands into 64 ui bands
                      if(uiBand != lastUiBand) {
                        lastUiBand = uiBand;
                        uiBandCount = 0;
                        uiBandMagnitude = 0.0;
                      }
                      uiBandMagnitude += magnitude1 / 64.0;
                      uiBandCount++;
                      
                      //fprintf(stderr, "%hu %i %i %f \n",uiFrameIndex / uiFrameStepSize,uiBand,uiBandCount,uiBandMagnitude);
                      histogram[uiFrameIndex / uiFrameStepSize][uiBand] = uiBandMagnitude / uiBandCount;                  
                    }


                    lastPhaseDomain1[bandIndex] = phaseDomain1[bandIndex];
                    if(outValue1.i < phaseThreshold || outValue1.r < phaseThreshold) {
                        phaseDomain1[bandIndex] = 0;
                    } else {
                        phaseDomain1[bandIndex] = atan2(outValue1.i, outValue1.r);
                    }
                    phaseDifference1[bandIndex] = phaseDomain1[bandIndex] - lastPhaseDomain1[bandIndex];

                    if(channels > 1) {
                        kiss_fft_cpx outValue2 = fft2->out[bandIndex]; 

                        //store magnitude
                        float magnitude2 = sqrt(outValue2.r * outValue2.r + outValue2.i * outValue2.i);
                        freqDomain2[bandIndex] = magnitude2;

                        lastPhaseDomain2[bandIndex] = phaseDomain2[bandIndex];
                        if(outValue2.i < phaseThreshold || outValue2.r < phaseThreshold) {
                            phaseDomain2[bandIndex] = 0;
                        } else {
                            phaseDomain2[bandIndex] = atan2(outValue2.i, outValue2.r);
                        }
                        phaseDifference2[bandIndex] = phaseDomain2[bandIndex] - lastPhaseDomain2[bandIndex];
                    }
                }
                uiFrameIndex++;
                binnings1->topN(MAX_VOICE_COUNT, freqDomain1, phaseDifference1, bins1, (FFTSortMode) 0);                
                //ResultArray binResults;
                for(int v=0;v<MAX_VOICE_COUNT;v++) {
                    //binResults.results[v] = bins1[v];
                    voiceAnalysis1.push_back(bins1[v]);
                }

                if(channels > 1) {
                    binnings2->topN(MAX_VOICE_COUNT, freqDomain2, phaseDifference2, bins2, (FFTSortMode) 0);                
                    //ResultArray binResults;
                    for(int v=0;v<MAX_VOICE_COUNT;v++) {
                        //binResults.results[v] = bins2[v];
                        voiceAnalysis2.push_back(bins2[v]);
                    }
                }


                //fprintf(stderr, "Size: %i \n",voiceAnalysis.size());

            }
        }
    }

    frameCount = voiceAnalysis1.size() / MAX_VOICE_COUNT;
    sampleStatusDesc = fileDesc + std::to_string(frameCount)+ " Frames\nWindow Function: " + windowFunction->windowFunctionName[windowFunctionId] + "\n";

    analysisStatus = 2; // 2 is completed
};


void FreudianSlipModule::process(const ProcessArgs &args) {
    if((analysisStatus == 0 || lastWindowFunctionId != windowFunctionId) && fileLoaded) {
        auto handle = std::async(std::launch::async, &FreudianSlipModule::analyze,this);
        lastWindowFunctionId = windowFunctionId;
        return;
    }

    if (positionModeTrigger.process(params[POSITION_MODE_PARAM].getValue())) {
        positionMode = !positionMode;
    }
    lights[POSITION_MODE_LIGHT+0].value = positionMode ? 0.0 : 1.0;
    lights[POSITION_MODE_LIGHT+1].value = positionMode ? 0.0 : 1.0;
    lights[POSITION_MODE_LIGHT+2].value = positionMode ? 0.0 : 0.2;

    if (eocModeTrigger.process(params[EOC_MODE_PARAM].getValue())) {
        eocMode = !eocMode;
    }
    lights[EOC_MODE_LIGHT+0].value = eocMode ? 0.8 : 0.0;
    lights[EOC_MODE_LIGHT+1].value = eocMode ? 0.4 : 0.6;
    lights[EOC_MODE_LIGHT+2].value = 0.0;


    if (loopTrigger.process(params[LOOP_PARAM].getValue() + inputs[LOOP_INPUT].getVoltage())) {
        loopMode = !loopMode;
    }
    lights[LOOP_MODE_LIGHT+0].value = loopMode;
    lights[LOOP_MODE_LIGHT+1].value = loopMode;
    lights[LOOP_MODE_LIGHT+2].value = loopMode ? 0.2 : 0.0;


    if (warpUseFundamentalTrigger.process(params[FREQ_WARP_USE_FUNDAMENTAL].getValue())) {
        warpBasedOnFundamental = !warpBasedOnFundamental;
    }
    lights[FREQ_WARP_USE_FUNDAMENTAL_LIGHT+0].value = warpBasedOnFundamental;
    lights[FREQ_WARP_USE_FUNDAMENTAL_LIGHT+1].value = warpBasedOnFundamental;
    lights[FREQ_WARP_USE_FUNDAMENTAL_LIGHT  +2].value = warpBasedOnFundamental ? 0.2 : 0.0;


    if(analysisStatus != 2 || !fileLoaded) { //Bail if nothing done yet
        return;
    }


    if (playTrigger.process(inputs[PLAY_INPUT].getVoltage())) {
        play = true;
        samplesPlayed = 0;
        for(int v=0;v<MAX_VOICE_COUNT;v++) {
            frameIndex[v]= frameCount * startPosition[v]; 
            framePlayedCount[v] = 0;
            frameSubIndex[v] = 0.0;
            framesCompleted[v] = false;     
        }
    }

    // here check to see if the frameSize or the sample rate have changed
    if (args.sampleRate != sampleRate) {
        sampleRate = args.sampleRate;
    }


    float fmValue[MAX_POLYPHONY * 2]{0};
    float amValue[MAX_POLYPHONY * 2]{0};
    uint8_t voiceCount = (uint8_t) paramValue(VOICE_COUNT, VOICE_COUNT_CV, 1, 32);
    // set the voice percentage
    voiceCountPercentage = (float(voiceCount - 1) / 31.0);

    // figure out any V_OCT changes
    float vOct = paramValue(V_OCT_PARAM, V_OCT_CV, -3, 3);
    vOctPercentage = vOct / 3.0;
    float vOctMultiplier = vOct >= 0 ? 1.0 + vOct : (1.0 / (1.0-vOct));

    // get the voice waveform to use
    uint8_t voiceWaveform = (uint8_t) paramValue(VOICE_WAVEFORM, VOICE_WAVEFORM_CV, 0, 4);
    // set the shape percentage
    waveformPercentage = (float(voiceWaveform) / 4.0);



    float fmShiftX = inputs[FM_SHIFT_X_CV].getVoltage() / 5.0;
    float fmShiftY = inputs[FM_SHIFT_Y_CV].getVoltage() / 5.0;
    frequencyModulationCells->shiftX = fmShiftX;
    frequencyModulationCells->shiftY = fmShiftY;

    float fmAmountShiftX = inputs[FM_AMOUNT_SHIFT_X_CV].getVoltage() / 5.0 + params[FM_AMOUNT].getValue();
    float fmAmountShiftY = inputs[FM_AMOUNT_SHIFT_Y_CV].getVoltage() / 5.0;
    frequencyModulationAmountCells->shiftX = fmAmountShiftX;
    frequencyModulationAmountCells->shiftY = fmAmountShiftY;
    fmAmountPercentage = params[FM_AMOUNT].getValue();

    float rmShiftX = inputs[RM_SHIFT_X_CV].getVoltage() / 5.0;
    float rmShiftY = inputs[RM_SHIFT_Y_CV].getVoltage() / 5.0;
    ringModulationCells->shiftX = rmShiftX;
    ringModulationCells->shiftY = rmShiftY;

    float rmMixShiftX = inputs[RM_MIX_SHIFT_X_CV].getVoltage() / 5.0  + params[RM_MIX].getValue();
    float rmMixShiftY = inputs[RM_MIX_SHIFT_Y_CV].getVoltage() / 5.0;
    ringModulationMixCells->shiftX = rmMixShiftX;
    ringModulationMixCells->shiftY = rmMixShiftY;
    rmMixPercentage = params[RM_MIX].getValue();

    float startPositionShiftX = inputs[START_POS_SHIFT_X_CV].getVoltage() / 5.0  + params[START_POS_PARAM].getValue();
    float startPositionShiftY = inputs[START_POS_SHIFT_Y_CV].getVoltage() / 5.0;
    float startPositionRotateX = inputs[START_POS_ROTATE_X_CV].getVoltage() / 5.0;
    startPositionCells->shiftX = startPositionShiftX;
    startPositionCells->shiftY = startPositionShiftY;
    startPositionCells->rotateX = startPositionRotateX;
    startPositionPercentage = params[START_POS_PARAM].getValue();

    float stopPositionShiftX = inputs[STOP_POS_SHIFT_X_CV].getVoltage() / 5.0  + params[STOP_POS_PARAM].getValue();
    float stopPositionShiftY = inputs[STOP_POS_SHIFT_Y_CV].getVoltage() / 5.0;
    float stopPositionRotateX = inputs[STOP_POS_ROTATE_X_CV].getVoltage() / 5.0;
    stopPositionCells->shiftX = stopPositionShiftX;
    stopPositionCells->shiftY = stopPositionShiftY;
    stopPositionCells->rotateX = stopPositionRotateX;
    stopPositionPercentage = params[STOP_POS_PARAM].getValue() + 1.0;

    float playSpeedShiftX = inputs[PLAY_SPEED_SHIFT_X_CV].getVoltage() / 5.0 + params[PLAY_SPEED_PARAM].getValue() / 6.0;
    float playSpeedShiftY = inputs[PLAY_SPEED_SHIFT_Y_CV].getVoltage() / 5.0;
    float playSpeedRotateX = inputs[PLAY_SPEED_ROTATE_X_CV].getVoltage() / 5.0;
    playSpeedCells->shiftX = playSpeedShiftX;
    playSpeedCells->shiftY = playSpeedShiftY;
    playSpeedCells->rotateX = playSpeedRotateX;
    playSpeedPercentage = params[PLAY_SPEED_PARAM].getValue() / 3.0;

    float panShiftX = inputs[PAN_SHIFT_X_CV].getVoltage() / 5.0;
    float panShiftY = inputs[PAN_SHIFT_Y_CV].getVoltage() / 5.0;
    float panRotateX = inputs[PAN_ROTATE_X_CV].getVoltage() / 5.0;
    panningCells->shiftX = panShiftX;
    panningCells->shiftY = panShiftY;
    panningCells->rotateX = panRotateX;

    //get cell changes
    if(samplesPlayed % 1024 == 0) {
        for (uint8_t voiceIndex = 0; voiceIndex < MAX_VOICE_COUNT; voiceIndex++) {
            fmMatrix[voiceIndex] = frequencyModulationCells->displayValueForPosition(voiceIndex);
            fmAmount[voiceIndex] = frequencyModulationAmountCells->valueForPosition(voiceIndex);
            rmMatrix[voiceIndex] = ringModulationCells->displayValueForPosition(voiceIndex);
            rmMix[voiceIndex] = ringModulationMixCells->valueForPosition(voiceIndex);
            panning[voiceIndex] = panningCells->valueForPosition(voiceIndex);
            playSpeed[voiceIndex] = playSpeedCells->valueForPosition(voiceIndex);
            startPosition[voiceIndex]= startPositionCells->valueForPosition(voiceIndex);
            stopPosition[voiceIndex]= stopPositionCells->valueForPosition(voiceIndex);
        }
        samplesPlayed =0;
    }
    samplesPlayed ++;



    randomizeStepAmount = paramValue(RANDOMIZE_PARAM, RANDOMIZE_CV, 0, 1.0);
    randomizeStepPercentage = randomizeStepAmount;

    freqWarpAmount = paramValue(FREQ_WARP_AMOUNT, FREQ_WARP_AMOUNT_CV, 0, 1.0);
    freqWarpCenterPercentage = paramValue(FREQ_WARP_CENTER, FREQ_WARP_CENTER_CV, 0, 1.0);
    freqWarpCenterFrequency = freqWarpCenterPercentage * 10000;

    if(play && analysisStatus == 2) {
        float randomStep = (((float) rand()/RAND_MAX) * randomizeStepAmount * frameCount / 100.0) - (randomizeStepAmount * float(frameCount) / 200.0); // Calculate it once so every voice is in sync, tends to go forward, but not always
            //fprintf(stderr, "%f\n", randomStep);
        bool allFramesPlayed = !eocMode;
        for(int v=0;v<MAX_VOICE_COUNT;v++) {
            //fprintf(stderr, "%i %i %f %i\n", loopMode, frameIndex[v], playSpeed[v], frameCount);
            if(!framesCompleted[v]) {
                frameSubIndex[v] += playSpeed[v];
                if(frameSubIndex[v] >= hopSizeInBytes) {
                    bankL.switchBanks(v);
                    bankR.switchBanks(v);
                    frameSubIndex[v] = 0;     
                    frameIndex[v] +=(1+randomStep); 
                    if(frameIndex[v] >= frameCount) {
                        frameIndex[v] -= frameCount;
                    }
                    if(frameIndex[v] < 0) {
                        frameIndex[v] += frameCount;
                    }
                    int32_t distanceToTravel;
                    if(positionMode) {
                        distanceToTravel= playSpeed[v] >=0 ?
                        (stopPosition[v] > startPosition[v] ? frameCount * (stopPosition[v]-startPosition[v]) :  frameCount * (1.0 - startPosition[v]+stopPosition[v])) :
                        (stopPosition[v] > startPosition[v] ? frameCount * (1.0 - stopPosition[v]+startPosition[v]) :  frameCount * (startPosition[v] - stopPosition[v])); 
                    } else {
                        distanceToTravel = frameCount * abs(stopPosition[v] - startPosition[v]);
                    }

                    framePlayedCount[v]+=1;
                    if(framePlayedCount[v] >= distanceToTravel) {
                        framesCompleted[v] = true;
                    }
                }
                if(frameSubIndex[v] < 0) {
                    bankL.switchBanks(v);
                    bankR.switchBanks(v);
                    frameSubIndex[v] = hopSizeInBytes-1;     
                    frameIndex[v] -= (1+randomStep); 
                    if(frameIndex[v] >= frameCount) {
                        frameIndex[v] -= frameCount;
                    }
                    if(frameIndex[v] < 0) {
                        frameIndex[v] += frameCount;
                    }
                    int32_t distanceToTravel;
                    if(positionMode) {
                        distanceToTravel= playSpeed[v] >=0 ?
                        (stopPosition[v] > startPosition[v] ? frameCount * (stopPosition[v]-startPosition[v]) :  frameCount * (1.0 - startPosition[v]+stopPosition[v])) :
                        (stopPosition[v] > startPosition[v] ? frameCount * (1.0 - stopPosition[v]+startPosition[v]) :  frameCount * (startPosition[v] - stopPosition[v])); 
                    } else {
                        distanceToTravel = frameCount * abs(stopPosition[v] - startPosition[v]);
                    }
                    
                    framePlayedCount[v]+=1;
                    if(framePlayedCount[v] >= distanceToTravel) {
                        framesCompleted[v] = true;
                    }
                }
            }
            allFramesPlayed = eocMode ? allFramesPlayed || framesCompleted[v] : allFramesPlayed && framesCompleted[v];
        }

        if(allFramesPlayed) {
            endOfSamplePulse.trigger(1e-3);
            if(!loopMode) {
                play = false;
            } else {
                samplesPlayed = 0;
                for(int v=0;v<MAX_VOICE_COUNT;v++) {
                    frameIndex[v]= frameCount * startPosition[v]; 
                    framePlayedCount[v] = 0;
                    frameSubIndex[v] = 0.0;
                    framesCompleted[v] = false;     
                }
            }
            return;
        }

        //Parallel version
        float_4 frequency1 = 0;
        float_4 frequency2 = 0;
        float_4 magnitude1 = 0;
        float_4 magnitude2 = 0;
        for (uint8_t i = 0, c = 0; i < voiceCount; ) {
            for (c = 0; c < 4; c++) {
                uint8_t o = i+c;
                if (o < voiceCount) {
                    uint32_t voiceIndex = frameIndex[o] * MAX_VOICE_COUNT + o;
                    if(!framesCompleted[o]) {
                        Result frameResults1 = voiceAnalysis1[voiceIndex];
                        frequency1[c] = frameResults1.frequency;
                        magnitude1[c] = frameResults1.magnitude;

                        if(channels > 1) {
                            Result frameResults2 = voiceAnalysis2[voiceIndex];
                            frequency2[c] = frameResults2.frequency;   
                            magnitude2[c] = frameResults2.magnitude;                            
                        }
                    }                    
                }                
            }

            frequency1 = frequency1 * vOctMultiplier;
            frequency1 = simd::clamp(frequency1 + (freqWarpAmount * ((frequency1 - freqWarpCenterFrequency))),1.0f,20000.0f);
            magnitude1 = magnitude1 / 512.0;

            if(channels > 1) {
                frequency2 = frequency2 * vOctMultiplier;
                frequency2 = simd::clamp(frequency2 + (freqWarpAmount * ((frequency2 - freqWarpCenterFrequency))),1.0f,20000.0f);
                magnitude2 = magnitude2 / 512.0;                            
            }

            for (c = 0; c < 4; c++) {
                uint8_t o = i+c;
                if (o < voiceCount) {
                    frequencies1[o] = frequency1[c];
                    magnitudes1[o] = magnitude1[c];
                    if(channels > 1) {
                        frequencies2[o] = frequency2[c];
                        magnitudes2[o] = magnitude2[c];                            
                    }
                } else {
                    magnitudes1[o] = 0;
                    magnitudes2[o] = 0;
                }
            }
            i += c;
        }


        bankL.setFrequency(frequencies1, magnitudes1, voiceCount);
        if(channels > 1)
            bankR.setFrequency(frequencies2, magnitudes2, voiceCount);

        int voiceIndex = 0;
        uint8_t fmCount1 = inputs[FM_INPUT_1].getChannels();
        for (uint8_t i = 0; i < fmCount1; i++) {
            fmValue[voiceIndex] = inputs[FM_INPUT_1].getVoltage(i) * 2000; //Converting -5 to 5 to -10000hz to +10000hz
            voiceIndex++;
        }
        uint8_t fmCount2 = inputs[FM_INPUT_2].getChannels();
        for (uint8_t i = 0; i < fmCount2; i++) {
            fmValue[voiceIndex] = inputs[FM_INPUT_2].getVoltage(i) * 2000; //Converting -5 to 5 to -10000hz to +10000hz
            voiceIndex++;
        }
        bankL.setFM(fmMatrix, fmAmount,fmValue, fmCount1 + fmCount2);
        bankR.setFM(fmMatrix, fmAmount,fmValue, fmCount1 + fmCount2);

        voiceIndex = 0;
        uint8_t amCount1 = inputs[AM_RM_INPUT_1].getChannels();
        for (uint8_t i = 0; i < amCount1; i++) {
            amValue[voiceIndex] = inputs[AM_RM_INPUT_1].getVoltage(i); 
            voiceIndex++;
        }
        uint8_t amCount2 = inputs[AM_RM_INPUT_2].getChannels();
        for (uint8_t i = 0; i < amCount2; i++) {
            amValue[voiceIndex] = inputs[AM_RM_INPUT_2].getVoltage(i); 
            voiceIndex++;
        }
        if(voiceIndex > 0) {
            bankL.setRM(rmActive,rmMatrix,rmMix,amValue,voiceIndex);
            bankR.setRM(rmActive,rmMatrix,rmMix,amValue,voiceIndex);
        }

        BankOutput output1 = bankL.process(voiceWaveform, args.sampleTime, panning);

        BankOutput output2;
        if(channels > 1)
            output2 = bankR.process(voiceWaveform, args.sampleTime, panning);


        //assert(!isnan(output.outputMono));

        if(!outputs[OUTPUT_R].isConnected()) { // mono mode
            outputs[OUTPUT_L].setVoltage(output1.outputMono + output2.outputMono);
        } else {
            if(channels == 1) {
                outputs[OUTPUT_L].setVoltage(output1.outputLeft);
                outputs[OUTPUT_R].setVoltage(output1.outputRight);
            } else {
                outputs[OUTPUT_L].setVoltage(output1.outputLeft);
                outputs[OUTPUT_R].setVoltage(output2.outputRight);
            }
        }
    }

    outputs[EOC_OUTPUT].setVoltage(endOfSamplePulse.process(1.0 / args.sampleRate) ? 10.0 : 0);	


//   for (uint8_t i = 0; i < 16; i++) {
//     outputs[DEBUG_OUTPUT].setVoltage(debugOutput[i], i);
//   }
//  outputs[DEBUG_OUTPUT].setChannels(16);
}
