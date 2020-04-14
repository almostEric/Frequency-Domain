#include "HarmonicConvergence.hpp"
#include "../component/tooltip.hpp"
#include <cmath>
#include "../model/Interpolate.hpp"


HarmonicConvergenceModule::HarmonicConvergenceModule() {
  config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

  // this is SOOOOO hacky

  configParam(VOICE_COUNT, 1.0f, 36.0f, 18.0f,"# of Voices");
  configParam<VCOVoice>(VOICE_WAVEFORM, 0.0f, 4.0f, 0.0f,"Voice Waveform");
  configParam(OCTAVE, -3.0f, 3.0f, 0.0f, "Frequency Shift (Octaves)");
  configParam(FREQ_WARP_AMOUNT, 0.0f, 1.0f, 0.0f,"Frequency Warp Amount","%",0,100);
  configParam(FREQ_WARP_CENTER, 0.0f, 1.0f, 0.0f,"Frequency Warp Center", " Hz", 0, 10000);
  configParam<SpectralModeName>(SPECTRAL_MODE, 0.0f, 3.0f, 0.0f, "Spectral Mode");
  configParam(VOICE_SHIFT, -35.0f, 35.0f, 0.0f, "Voice Shift");

  configParam(MIX, 0.0f, 100.0f, 50.0f,"Wet Mix","%");
  configParam(FRAME_SIZE, 7.0f, 13.0f, 9.0f, "Frame Size"," Bytes",2,1);

  configParam(MORPH, -1.0f, 1.0f, 0.0f, "Morph","%",0,100);
  configParam(FM_AMOUNT, 0.0f, 1.0f, 0.0f, "FM Amount","%",0,100);
  configParam(RM_MIX, 0.0f, 1.0f, 0.0f, "RM Mix","%",0,100);

  configParam(RING_MODULATION, 0.0f, 1.0f, 0.0f, "Ring Modulation");



  // frameSize / sampleRate
  frameSize1 = 9;
  lastFrameSize1 = frameSize1;
  hopSize1 = frameSize1 - 2;
  frameSizeInBytes1 = pow(2.0f, frameSize1);
  hopSizeInBytes1 = pow(2.0f, hopSize1);

  frameSize2 = 9;
  lastFrameSize2 = frameSize2;
  hopSize2 = frameSize2 - 2;
  frameSizeInBytes2 = pow(2.0f, frameSize2);
  hopSizeInBytes2 = pow(2.0f, hopSize2);

  sampleRate = APP->engine->getSampleRate();
  
  // window function
  windowFunctionId = 4;

  lpf1.setType(bq_type_lowpass);
  hpf1.setType(bq_type_highpass);
  lpf2.setType(bq_type_lowpass);
  hpf2.setType(bq_type_highpass);


  fft1 = new FFT(frameSizeInBytes1);
  fft2 = new FFT(frameSizeInBytes2);

  binnings1 = new Binning(frameSizeInBytes1, sampleRate);
  binnings2 = new Binning(frameSizeInBytes2, sampleRate);

  windowFunction1 = new WindowFunction<float>(frameSizeInBytes1);
  windowFunction2 = new WindowFunction<float>(frameSizeInBytes2);
  dry1 = new Buffer<float>(pow(2.0f, 13), 0);
  dry2 = new Buffer<float>(pow(2.0f, 13), 0);

  for (uint8_t i = 0; i < MAX_FRAMES; i++) {
    dryBuffer1[i] = new Buffer<float>(frameSizeInBytes1, hopSizeInBytes1 * i);
    dryBuffer2[i] = new Buffer<float>(frameSizeInBytes2, hopSizeInBytes2 * i);
  }

  // set up one storage for matrix displays
  morphingCells = new OneDimensionalCellsWithRollover(40, 36, 0, 1, PIN_ROLLOVER_MODE, WRAP_AROUND_ROLLOVER_MODE,0.5);
  frequencyModulationCells = new OneDimensionalCellsWithRollover(32, 36, 0 , 1, PIN_ROLLOVER_MODE, WRAP_AROUND_ROLLOVER_MODE);
  frequencyModulationAmountCells = new OneDimensionalCellsWithRollover(40, 36, 0, 1,PIN_ROLLOVER_MODE, WRAP_AROUND_ROLLOVER_MODE);
  ringModulationCells = new OneDimensionalCellsWithRollover(36, 36, 0, 1, PIN_ROLLOVER_MODE, WRAP_AROUND_ROLLOVER_MODE);
  ringModulationMixCells = new OneDimensionalCellsWithRollover(40, 36, 0, 1,PIN_ROLLOVER_MODE, WRAP_AROUND_ROLLOVER_MODE);
  panningCells = new OneDimensionalCellsWithRollover(40, 36, -1, 1, PIN_ROLLOVER_MODE, WRAP_AROUND_ROLLOVER_MODE);
}

HarmonicConvergenceModule::~HarmonicConvergenceModule() {
  delete dry1;
  delete dry2;
  for (uint8_t i = 0; i < MAX_FRAMES; i++) {
    if (dryBuffer1[i]) {
      delete dryBuffer1[i];
    }
    if (dryBuffer2[i]) {
      delete dryBuffer2[i];
    }
  }

  // delete biquadFilter1;
  // delete biquadFilter2;

  delete windowFunction1;
  delete windowFunction2;

  delete fft1;
  delete fft2;

  delete binnings1;
  delete binnings2;

  delete morphingCells;
  delete frequencyModulationCells;
  delete frequencyModulationAmountCells;
  delete ringModulationCells;
  delete ringModulationMixCells;
  delete panningCells;
}

void HarmonicConvergenceModule::onReset() {
  // window function
  windowFunctionId = 4;

  // reset the cells
  morphingCells->reset();
  frequencyModulationCells->reset();
  frequencyModulationAmountCells->reset();
  ringModulationCells->reset();
  ringModulationMixCells->reset();
  panningCells->reset();

  // ring modulator off
  rmActive = false;
}

void HarmonicConvergenceModule::dataFromJson(json_t *root) {
  json_t *fs1 = json_object_get(root, "frameSize1");
  if (json_is_integer(fs1)) {
    frameSize1 = (uint8_t) json_integer_value(fs1);
  }
  json_t *fs2 = json_object_get(root, "frameSize2");
  if (json_is_integer(fs2)) {
    frameSize2 = (uint8_t) json_integer_value(fs2);
  }
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
    buf = "morphing-" + std::to_string(i) ;
    json_t *morphJ = json_object_get(root, buf.c_str());
    if (morphJ) {
      morphingCells->cells[i] = json_real_value(morphJ);
    }
    buf = "panning-" + std::to_string(i) ;
    json_t *panJ = json_object_get(root, buf.c_str());
    if (panJ) {
      panningCells->cells[i] = json_real_value(panJ);
    }
  }
}

json_t *HarmonicConvergenceModule::dataToJson() {
  json_t *root = json_object();
  json_object_set(root, "frameSize1", json_integer(frameSize1));
  json_object_set(root, "frameSize2", json_integer(frameSize2));
  json_object_set(root, "windowFunction", json_integer(windowFunctionId));
  json_object_set(root, "rmActive", json_boolean(rmActive));
  json_object_set(root, "warpBasedOnFundamental", json_integer(warpBasedOnFundamental));
  for(int i=0;i<MAX_VOICE_COUNT;i++) {
    std::string buf = "fmMatrix-" + std::to_string(i) ;
    json_object_set(root, buf.c_str(),json_real((float) frequencyModulationCells->cells[i]));
    buf = "fmAmount-" + std::to_string(i) ;
    json_object_set(root, buf.c_str(),json_real((float) frequencyModulationAmountCells->cells[i]));
    buf = "rmMatrix-" + std::to_string(i) ;
    json_object_set(root, buf.c_str(),json_real((float) ringModulationCells->cells[i]));
    buf = "rmMix-" + std::to_string(i) ;
    json_object_set(root, buf.c_str(),json_real((float) ringModulationMixCells->cells[i]));
    buf = "morphing-" + std::to_string(i) ;
    json_object_set(root, buf.c_str(),json_real((float) morphingCells->cells[i]));
    buf = "panning-" + std::to_string(i) ;
    json_object_set(root, buf.c_str(),json_real((float) panningCells->cells[i]));
  }

  return root;
}

float HarmonicConvergenceModule::paramValue (uint16_t param, uint16_t input, float low, float high) {
  float current = params[param].getValue();

  if (inputs[input].isConnected()) {
    // high - low, divided by one tenth input voltage, plus the current value
    current += ((inputs[input].getVoltage() / 10) * (high - low));
  }

  return clamp(current, low, high);
}


float HarmonicConvergenceModule::lowFreq() {
  // float bandwidth = 20000;
  // float center = 5000;

//  return clamp(center - bandwidth / 2.0, 20, 20000);
    return 50;
}

float HarmonicConvergenceModule::highFreq() {
  // float bandwidth = 20000;
  // float center = 5000;

  //return clamp(center + bandwidth / 2.0, 20, 20000);
  return 15000;
}


void HarmonicConvergenceModule::process(const ProcessArgs &args) {
  // get the current input

  input1Connected = inputs[INPUT_1].isConnected();
  input2Connected = inputs[INPUT_2].isConnected();

  float input1 = inputs[INPUT_1].getVoltage();
  float input2 = inputs[INPUT_2].getVoltage();
  // set the real value of the dry input
  dry1->set(input1);
  dry2->set(input2);

  if (rmTrigger.process(params[RING_MODULATION].getValue())) {
    rmActive = !rmActive;
  }
  lights[RING_MODULATION_ENABLED_LIGHT+0].value = rmActive;
  lights[RING_MODULATION_ENABLED_LIGHT+1].value = rmActive;
  lights[RING_MODULATION_ENABLED_LIGHT+2].value = rmActive ? 0.2 : 0.0;

  if (warpUseFundamentalTrigger.process(params[FREQ_WARP_USE_FUNDAMENTAL].getValue())) {
    warpBasedOnFundamental = !warpBasedOnFundamental;
  }
  lights[FREQ_WARP_USE_FUNDAMENTAL_LIGHT+0].value = warpBasedOnFundamental;
  lights[FREQ_WARP_USE_FUNDAMENTAL_LIGHT+1].value = warpBasedOnFundamental;
  lights[FREQ_WARP_USE_FUNDAMENTAL_LIGHT  +2].value = warpBasedOnFundamental ? 0.2 : 0.0;


  // get the dry input from the buffer, it should end up one loop delayed
  float dryDelayed = dry1->get(); // use a real dry value, not something that we have altered
  float dryDelayed2 = dry2->get(); // use a real dry value, not something that we have altered 

  for(uint8_t i=0; i < MAX_FRAMES; i++) {
    float windowedValue1 = windowFunction1->windowValue(windowFunctionId, dryBuffer1[i]->setPos);
    float windowedValue2 = windowFunction2->windowValue(windowFunctionId, dryBuffer2[i]->setPos);
    // debugOutput[0] = input1;
    // debugOutput[1] = input1 * windowedValue1;

      // set the hpf and lpf values for filtering of bandwidth/center
    hpf1.setFc(lowFreq() / args.sampleRate);
    lpf1.setFc(highFreq() / args.sampleRate);
    hpf1.calcBiquad();
    lpf1.calcBiquad();

    hpf2.setFc(lowFreq() / args.sampleRate);
    lpf2.setFc(highFreq() / args.sampleRate);
    hpf2.calcBiquad();
    lpf2.calcBiquad();

    //TODO: Take advantage of SIMD

    if(input1Connected) {
      input1 = hpf1.process(lpf1.process(input1))[0];
      dryBuffer1[i]->set(input1 * windowedValue1);
    }

    if(input2Connected) {
      input2 = hpf2.process(lpf2.process(input2))[0];
      dryBuffer2[i]->set(input2 * windowedValue2);
    }

    bool updateCells = false;
    // is it time to do the fft?
    if (dryBuffer1[i]->looped) {
      updateCells = true;
      //Do FFT, then calculate moving averages
      fft1->fft(dryBuffer1[i]->data);

      for(uint16_t bandIndex=0;bandIndex<=frameSizeInBytes1 / 2;bandIndex++) {

        kiss_fft_cpx outValue1 = fft1->out[bandIndex]; 

        //store magnitude
        outBuffer1[i][bandIndex] = sqrt(outValue1.r * outValue1.r + outValue1.i * outValue1.i);
        //debugOutput[2] = sqrt(outValue1.r * outValue1.r + outValue1.i * outValue1.i);

        float aveMagnitude1 = 0;
        for(uint8_t frameIndex=0;frameIndex<MAX_FRAMES;frameIndex++) {
            aveMagnitude1 += outBuffer1[frameIndex][bandIndex];
        }
        freqDomain1[bandIndex] = aveMagnitude1 / MAX_FRAMES;

        lastPhaseDomain1[bandIndex] = phaseDomain1[bandIndex];
        if(outValue1.i < phaseThreshold || outValue1.r < phaseThreshold) {
          phaseDomain1[bandIndex] = 0;
        } else {
          phaseDomain1[bandIndex] = atan2(outValue1.i, outValue1.r);
        }
        phaseDifference1[bandIndex] = phaseDomain1[bandIndex] - lastPhaseDomain1[bandIndex];
      }
    }

    if (dryBuffer2[i]->looped) {
      updateCells= true;
      //Do FFT, then calculate moving averages
      fft2->fft(dryBuffer2[i]->data);

      for(uint16_t bandIndex=0;bandIndex<=frameSizeInBytes2 / 2;bandIndex++) {

        kiss_fft_cpx outValue2 = fft2->out[bandIndex]; 

        //store magnitude
        outBuffer2[i][bandIndex] = sqrt(outValue2.r * outValue2.r + outValue2.i * outValue2.i);
        float aveMagnitude2 = 0;

        for(uint8_t frameIndex=0;frameIndex<MAX_FRAMES;frameIndex++) {
            aveMagnitude2 += outBuffer2[frameIndex][bandIndex];
        }
        freqDomain2[bandIndex] = aveMagnitude2 / MAX_FRAMES;

        lastPhaseDomain2[bandIndex] = phaseDomain2[bandIndex];
        if(outValue2.i < phaseThreshold || outValue2.r < phaseThreshold) {
          phaseDomain2[bandIndex] = 0;
        } else {
          phaseDomain2[bandIndex] = atan2(outValue2.i, outValue2.r);
        }
        phaseDifference2[bandIndex] = phaseDomain2[bandIndex] - lastPhaseDomain2[bandIndex];
      }            
    }

    //get cell changes
    if(updateCells) {
      for (uint8_t voiceIndex = 0; voiceIndex < MAX_VOICE_COUNT; voiceIndex++) {
        morphing[voiceIndex] = morphingCells->valueForPosition(voiceIndex);
        fmMatrix[voiceIndex] = frequencyModulationCells->displayValueForPosition(voiceIndex) / 2;
        fmAmount[voiceIndex] = frequencyModulationAmountCells->valueForPosition(voiceIndex);
        rmMatrix[voiceIndex] = ringModulationCells->displayValueForPosition(voiceIndex);
        rmMix[voiceIndex] = ringModulationMixCells->valueForPosition(voiceIndex);
        panning[voiceIndex] = panningCells->valueForPosition(voiceIndex);
      }

      // get the spectral mode
      uint8_t spectralMode = (uint8_t) paramValue(SPECTRAL_MODE, SPECTRAL_MODE_CV, 0, 3);
      spectralPercentage = float(spectralMode) / 3.0f;
      binnings1->topN(MAX_VOICE_COUNT, freqDomain1, phaseDifference1, bins1, (FFTSortMode) spectralMode);
      binnings2->topN(MAX_VOICE_COUNT, freqDomain2, phaseDifference2, bins2, (FFTSortMode) spectralMode);
      bank.switchBanks();
    }


    // here check to see if the frameSize or the sample rate have changed
    if (args.sampleRate != sampleRate) {
      sampleRate = args.sampleRate;
    }

    if (lastFrameSize1 != frameSize1) {
      lastFrameSize1 = frameSize1;
      hopSize1 = frameSize1 - 2;
      frameSizeInBytes1 = pow(2.0f, frameSize1);
      hopSizeInBytes1 = pow(2.0f, hopSize1);

      delete fft1;
      fft1 = new FFT(frameSizeInBytes1);

      delete binnings1;
      binnings1 = new Binning(frameSizeInBytes1, sampleRate);

      delete windowFunction1;
      windowFunction1 = new WindowFunction<float>(frameSizeInBytes1);

      for (uint8_t i = 0; i < MAX_FRAMES; i++) {
        delete dryBuffer1[i];
        dryBuffer1[i] = new Buffer<float>(frameSizeInBytes1, hopSizeInBytes1 * i);
      }
    }

    if (lastFrameSize2 != frameSize2) {
      lastFrameSize2 = frameSize2;
      hopSize2 = frameSize2 - 2;
      frameSizeInBytes2 = pow(2.0f, frameSize2);
      hopSizeInBytes2 = pow(2.0f, hopSize2);

      delete fft2;
      fft2 = new FFT(frameSizeInBytes2);

      delete binnings2;
      binnings2 = new Binning(frameSizeInBytes2, sampleRate);

      delete windowFunction2;
      windowFunction2 = new WindowFunction<float>(frameSizeInBytes2);

      for (uint8_t i = 0; i < MAX_FRAMES; i++) {
        delete dryBuffer2[i];
        dryBuffer2[i] = new Buffer<float>(frameSizeInBytes2, hopSizeInBytes2 * i);
      }
    }
  }

  float frequencies[MAX_VOICE_COUNT]{0};
  float magnitudes[MAX_VOICE_COUNT]{0};

  float fmValue[MAX_POLYPHONY]{0};
  uint8_t voiceCount = (uint8_t) paramValue(VOICE_COUNT, VOICE_COUNT_CV, 1, 36);
  // set the voice percentage
  voiceCountPercentage = (float(voiceCount - 1) / 35.0);

  // figure out any octave changes
  float octave = paramValue(OCTAVE, OCTAVE_CV, -2, 3);
  octavePercentage = octave / 3.0;

  float morphShiftX = inputs[MORPH_SHIFT_X_CV].getVoltage() / 5.0 + params[MORPH].getValue();
  float morphShiftY = inputs[MORPH_SHIFT_Y_CV].getVoltage() / 5.0;
  morphingCells->shiftX = morphShiftX;
  morphingCells->shiftY = morphShiftY;
  morphPercentage = params[MORPH].getValue();

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


  float panShiftX = inputs[PAN_SHIFT_X_CV].getVoltage() / 5.0;
  float panShiftY = inputs[PAN_SHIFT_Y_CV].getVoltage() / 5.0;
  panningCells->shiftX = panShiftX;
  panningCells->shiftY = panShiftY;


  freqWarpAmount = paramValue(FREQ_WARP_AMOUNT, FREQ_WARP_AMOUNT_CV, 0, 1.0);
  freqWarpCenterPercentage = paramValue(FREQ_WARP_CENTER, FREQ_WARP_CENTER_CV, 0, 1.0);
  freqWarpCenterFrequency = freqWarpCenterPercentage * 10000;
  
  //magnitudeNormaliztion = 2.0 / powf(windowFunction->sum[windowFunctionId], 2.0);
  for (uint8_t i = 0; i < voiceCount; i++) {

    float frequency1,frequency2,magnitude1,magnitude2;
    if (octave == 0) {
      frequency1 = bins1[i].frequency;
      frequency2 = bins2[i].frequency;
    } else if (octave < 0) {
      frequency1 = clamp(bins1[i].frequency * (1.0 + octave / 2.0), 20.0f, 20000.0f);
      frequency2 = clamp(bins2[i].frequency * (1.0 + octave / 2.0), 20.0f, 20000.0f);
    } else {
      frequency1 = clamp(bins1[i].frequency * (1.0 + octave), 20.0f, 20000.0f);
      frequency2 = clamp(bins2[i].frequency * (1.0 + octave), 20.0f, 20000.0f);
    } 

    float interpolatedFrequency;
    if(input1Connected && input2Connected) {
      interpolatedFrequency = interpolate(frequency1,frequency2,morphing[i],0.0f,1.0f);
    } else if (input2Connected) {
      interpolatedFrequency = frequency2;
    } else {
      interpolatedFrequency = frequency1;
    }
    if(i == 0 && warpBasedOnFundamental) {
      freqWarpCenterFrequency = interpolatedFrequency + freqWarpCenterFrequency;
    }
    float adjustedFrequency = clamp(interpolatedFrequency + (freqWarpAmount * ((interpolatedFrequency - freqWarpCenterFrequency))),1.0f,20000.0f);
    frequencies[i] = adjustedFrequency;

    magnitude1 = bins1[i].magnitude / magnitudeAdjustment[frameSize1 - 7];
    magnitude2 = bins2[i].magnitude / magnitudeAdjustment[frameSize2 - 7];
    // magnitude1 = bins1[i].magnitude * magnitudeNormaliztion;
    // magnitude2 = bins2[i].magnitude * magnitudeNormaliztion;
    magnitudes[i] = interpolate(magnitude1,magnitude2,morphing[i],0.0f,1.0f);
  }

  bank.setFrequency(frequencies, magnitudes, voiceCount);

  int fmCount = inputs[FM_INPUT].getChannels();
  for (uint8_t i = 0; i < fmCount; i++) {
    fmValue[i] = inputs[FM_INPUT].getVoltage(i) * 2000; //Converting -5 to 5 to -10000hz to +10000hz
  }
  bank.setFM(fmMatrix, fmAmount,fmValue, fmCount);

  bank.setRM(rmActive,rmMatrix,rmMix);


  bank.setVoiceShift((int8_t) paramValue(VOICE_SHIFT, VOICE_SHIFT_CV, 1-MAX_VOICE_COUNT, MAX_VOICE_COUNT-1));
  shiftPercentage = (paramValue(VOICE_SHIFT, VOICE_SHIFT_CV, 1-MAX_VOICE_COUNT, MAX_VOICE_COUNT-1)) / 35.0;

  // get the voice waveform to use
  uint8_t voiceWaveform = (uint8_t) paramValue(VOICE_WAVEFORM, VOICE_WAVEFORM_CV, 0, 4);
  // set the shape percentage
  waveformPercentage = (float(voiceWaveform) / 4.0);

  BankOutput output = bank.process(voiceWaveform, args.sampleTime, panning);

  float mix = paramValue(MIX, MIX_CV, 0, 100);
  // set the mix percentage
  mixPercentage = mix / 100.0;

//  assert(!isnan(output.outputMono));

  if(!outputs[OUTPUT_R].isConnected()) { // mono mode
    outputs[OUTPUT_L].setVoltage(interpolate(dryDelayed+dryDelayed2, output.outputMono, mix, 0.0f, 100.0f));
  } else {
    outputs[OUTPUT_L].setVoltage(interpolate(dryDelayed+dryDelayed2, output.outputLeft, mix, 0.0f, 100.0f));
    outputs[OUTPUT_R].setVoltage(interpolate(dryDelayed+dryDelayed2, output.outputRight, mix, 0.0f, 100.0f));
  }

  for (uint8_t i = 0; i < 16; i++) {
    outputs[DEBUG_OUTPUT].setVoltage(debugOutput[i], i);
  }
 outputs[DEBUG_OUTPUT].setChannels(16);
}
