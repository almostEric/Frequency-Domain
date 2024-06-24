#include "Morphology.hpp"

#include <vector>
#include <cmath>
#include "../model/Interpolate.hpp"

MorphologyModule::MorphologyModule() {
  config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

  configParam(SPREAD, -1024.0f, 1024.0f, 0.0f, "Band Spread");

  configParam(X_AXIS_PIN_POS_BAND_SPREAD, 0.0f, 1.0f, 0.0f, "Band Spread X Axis Pin Position","%",0,100);
  configParam(X_AXIS_PIN_POS_PANNING, 0.0f, 1.0f, 0.0f, "Panning X Axis Pin Position","%",0,100);

  configParam(X_AXIS_ROTATION_BAND_SPREAD, -1.0f, 1.0f, 0.0f, "Band Spread X Axis Rotation","°",0,180);
  configParam(X_AXIS_ROTATION_PANNING, -1.0f, 1.0f, 0.0f, "Panning X Axis Rotation","°",0,180);


  configParam(INVERT_THRESHOLD_1, 0.0f, 50.0f, 0.0f, "Invert Threshold L");
  configParam(INVERT_THRESHOLD_2, 0.0f, 50.0f, 0.0f, "Invert Threshold R");


  configButton(PIN_BAND_SPREAD_XS,"Band Spread Pin X Axis Mode");
  configButton(PIN_PANNING_XS,"Panning Pin X Axis Mode");

  configButton(INVERT_SPECTA_1,"Invert 1's Spectra");
  configButton(INVERT_SPECTA_2,"Invert 2's Spectra");


  configInput(INPUT_1, "#1");
  configInput(INPUT_2, "#2");

  configInput(BAND_SHIFT_X_CV, "Band Shift X CV");
  configInput(BAND_SHIFT_Y_CV, "Band Shift Y CV");

  configInput(PANNING_X_CV, "Panning X CV");
  configInput(PANNING_Y_CV, "Panning Y CV");

  configInput(SPREAD_CV, "Band Spread CV");

  configInput(X_AXIS_PIN_POS_BAND_SPREAD_CV, "Band Spread X Axis Pin CV");
  configInput(X_AXIS_PIN_POS_PANNING_CV, "Panning X Axis Pin CV");
  configInput(X_AXIS_ROTATION_BAND_SPREAD_CV, "Band Spread X Axis Rotation CV");
  configInput(X_AXIS_ROTATION_PANNING_CV, "Panning X Axis Rotation CV");

  configInput(INVERT_THRESHOLD_1_CV, "Invert Threshold 1 CV");
  configInput(INVERT_THRESHOLD_2_CV, "Invert Threshold 2 CV");

  configOutput(OUTPUT_L, "Left");
  configOutput(OUTPUT_R, "Right");
    

  frameSize = 11;
  hopSize = frameSize - 2;
  nbrBands = pow(2,frameSize-1);

  fft1 = new FFT(nbrBands * 2);
  fft2 = new FFT(nbrBands * 2);

  sampleRate = APP->engine->getSampleRate();
  currentBin = 0;

  windowFunction = new WindowFunction<float>(pow(2.0f, frameSize));
  dry1 = new Buffer<float>(pow(2.0f, frameSize), 0);
  dry2 = new Buffer<float>(pow(2.0f, frameSize), 0);
  for (uint8_t i = 0; i < MAX_FRAMES; i++) {
    dryBuffer1[i] = new Buffer<float>(pow(2.0f, frameSize),i * pow(2.0f,hopSize));
    dryBuffer2[i] = new Buffer<float>(pow(2.0f, frameSize),i * pow(2.0f,hopSize));
    curPos[i] = i * pow(2.0f,hopSize);    
  }

  bandShiftCells = new OneDimensionalCellsWithRollover(50, NUM_UI_BANDS, 0, 1, PIN_ROLLOVER_MODE, WRAP_AROUND_ROLLOVER_MODE);
  panningCells = new OneDimensionalCellsWithRollover(50, NUM_UI_BANDS, -1, 1, PIN_ROLLOVER_MODE, WRAP_AROUND_ROLLOVER_MODE,0);


  uint16_t bandMultiplier = pow(2,frameSize - 11);
  //Set up UI Bands
  uint16_t bandIndex = 0;
  for(uint8_t i = 0; i < 2;i++) {
    bandsPerUIBand[bandIndex++]= 128 * bandMultiplier;
  }
  for(uint8_t i = 0; i < 4;i++) {
    bandsPerUIBand[bandIndex++]= 64 * bandMultiplier;
  }
  for(uint8_t i = 0; i < 5;i++) {
    bandsPerUIBand[bandIndex++]= 32 * bandMultiplier;
  }
  for(uint8_t i = 0; i < 7;i++) {
    bandsPerUIBand[bandIndex++]= 16 * bandMultiplier;
  }
  for(uint8_t i = 0; i < 11;i++) {
    bandsPerUIBand[bandIndex++]= 8 * bandMultiplier;
  }
  for(uint8_t i = 0; i < 11;i++) {
    bandsPerUIBand[bandIndex++]= 4 * bandMultiplier;
  }
  for(uint8_t i = 0; i < 20;i++) {
    bandsPerUIBand[bandIndex++]= 2 * bandMultiplier;
  }
  for(uint8_t i = 0; i < 68;i++) {
    bandsPerUIBand[bandIndex++]= 1 * bandMultiplier;
  }

  onReset();
}

MorphologyModule::~MorphologyModule() {
  delete windowFunction;
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
}


void MorphologyModule::dataFromJson(json_t *root) {

  json_t *wfJ = json_object_get(root, "windowFunction");
  if (json_is_integer(wfJ)) {
    windowFunctionId = (uint8_t) json_integer_value(wfJ);
  }

  json_t *fsJ = json_object_get(root, "frameSize");
  if (json_is_integer(fsJ)) {
    frameSize = (uint8_t) json_integer_value(fsJ);
  }

  json_t *is1J = json_object_get(root, "invertSpectra1");
  if (json_is_integer(is1J)) {
    invertSpectra1 = (uint8_t) json_integer_value(is1J);
  }
  json_t *is2J = json_object_get(root, "invertSpectra2");
  if (json_is_integer(is2J)) {
    invertSpectra2 = (uint8_t) json_integer_value(is2J);
  }

  json_t *pbJ = json_object_get(root, "pinBandSpreadXs");
  if (json_is_integer(pbJ)) {
    pinBandSpreadXs = (uint8_t) json_integer_value(pbJ);
  }

  json_t *ppJ = json_object_get(root, "pinPanningXs");
  if (json_is_integer(ppJ)) {
    pinPanningXs = (uint8_t) json_integer_value(ppJ);
  }


  for(int i=0;i<NUM_UI_BANDS;i++) {
    std::string buf = "bandShift-" + std::to_string(i) ;
    json_t *attJ = json_object_get(root, buf.c_str());
    if (attJ) {
      bandShiftCells->cells[i] = json_real_value(attJ);
    }
    buf = "panning-" + std::to_string(i) ;
    json_t *dtJ = json_object_get(root, buf.c_str());
    if (dtJ) {
      panningCells->cells[i] = json_real_value(dtJ);
    }
  }
}

json_t *MorphologyModule::dataToJson() {
  json_t *root = json_object();

  json_object_set(root, "windowFunction", json_integer(windowFunctionId));
  json_object_set(root, "frameSize", json_integer(frameSize));

  json_object_set(root, "invertSpectra1", json_integer(invertSpectra1));
  json_object_set(root, "invertSpectra2", json_integer(invertSpectra2));
  json_object_set(root, "pinBandSpreadXs", json_integer(pinBandSpreadXs));
  json_object_set(root, "pinPanningXs", json_integer(pinPanningXs));
  for(int i=0;i<NUM_UI_BANDS;i++) {
    std::string buf = "bandShift-" + std::to_string(i) ;
    json_object_set(root, buf.c_str(),json_real((float) bandShiftCells->cells[i]));
    buf = "panning-" + std::to_string(i) ;
    json_object_set(root, buf.c_str(),json_real((float) panningCells->cells[i]));
  }

  return root;
}

void MorphologyModule::onReset() {

  windowFunctionId = 4;

  // reset the cells
  bandShiftCells->reset();
  panningCells->reset();
}

float MorphologyModule::paramValue (uint16_t param, uint16_t input, float low, float high) {
  float current = params[param].getValue();

  if (inputs[input].isConnected()) {
    // high - low, divided by one tenth input voltage, plus the current value
    current += ((inputs[input].getVoltage() / 10) * (high - low));
  }

  return clamp(current, low, high);
}

void MorphologyModule::process(const ProcessArgs &args) {
  // get the current input
  float input1 = inputs[INPUT_1].getVoltage();
  float input2 = inputs[INPUT_2].getVoltage();
  // set the real value of the dry input
  dry1->set(input1);
  dry1->set(input2);


  // here check to see if the frameSize or the sample rate have changed
  if (args.sampleRate != sampleRate) {
    sampleRate = args.sampleRate;
  }

  if (invertSpectra1Trigger.process(params[INVERT_SPECTA_1].getValue())) {
    invertSpectra1 = !invertSpectra1;
  }
  lights[INVERT_SPECTA_1_LIGHT+0].value = invertSpectra1;
  lights[INVERT_SPECTA_1_LIGHT+1].value = invertSpectra1;
  lights[INVERT_SPECTA_1_LIGHT+2].value = invertSpectra1 ? 0.2 : 0.0;

  if (invertSpectra2Trigger.process(params[INVERT_SPECTA_2].getValue())) {
    invertSpectra2 = !invertSpectra2;
  }
  lights[INVERT_SPECTA_2_LIGHT+0].value = invertSpectra2;
  lights[INVERT_SPECTA_2_LIGHT+1].value = invertSpectra2;
  lights[INVERT_SPECTA_2_LIGHT+2].value = invertSpectra2 ? 0.2 : 0.0;



  if (lastFrameSize != frameSize) {
    lastFrameSize = frameSize;
    hopSize = frameSize - 2;
    nbrBands = pow(2,frameSize-1);

    delete windowFunction;
    windowFunction = new WindowFunction<float>(pow(2.0f, frameSize));

    delete dry1;
    dry1 = new Buffer<float>(pow(2.0f, frameSize), 0);
    delete dry2;
    dry2 = new Buffer<float>(pow(2.0f, frameSize), 0);

    for (uint8_t i = 0; i < MAX_FRAMES; i++) {
      delete dryBuffer1[i];
      delete dryBuffer2[i];
      dryBuffer1[i] = new Buffer<float>(pow(2.0f, frameSize),i * pow(2.0f,hopSize));
      dryBuffer2[i] = new Buffer<float>(pow(2.0f, frameSize),i * pow(2.0f,hopSize));
      curPos[i] = i * pow(2.0f,hopSize); 
    }

    delete fft1;
    fft1 = new FFT(nbrBands * 2);
    delete fft2;
    fft2 = new FFT(nbrBands * 2);

    uint16_t bandMultiplier = pow(2,frameSize - 11);
    //Set up UI Bands
    uint16_t bandIndex = 0;
    for(uint8_t i = 0; i < 2;i++) {
      bandsPerUIBand[bandIndex++]= 128 * bandMultiplier;
    }
    for(uint8_t i = 0; i < 4;i++) {
      bandsPerUIBand[bandIndex++]= 64 * bandMultiplier;
    }
    for(uint8_t i = 0; i < 5;i++) {
      bandsPerUIBand[bandIndex++]= 32 * bandMultiplier;
    }
    for(uint8_t i = 0; i < 7;i++) {
      bandsPerUIBand[bandIndex++]= 16 * bandMultiplier;
    }
    for(uint8_t i = 0; i < 11;i++) {
      bandsPerUIBand[bandIndex++]= 8 * bandMultiplier;
    }
    for(uint8_t i = 0; i < 11;i++) {
      bandsPerUIBand[bandIndex++]= 4 * bandMultiplier;
    }
    for(uint8_t i = 0; i < 20;i++) {
      bandsPerUIBand[bandIndex++]= 2 * bandMultiplier;
    }
    for(uint8_t i = 0; i < 68;i++) {
      bandsPerUIBand[bandIndex++]= 1 * bandMultiplier;
    }

  }

  float pinXAxisPosBandSpread = paramValue(X_AXIS_PIN_POS_BAND_SPREAD, X_AXIS_PIN_POS_BAND_SPREAD_CV, 0, 1);
  bandSpreadXAxisPercentage = pinXAxisPosBandSpread;
  float xAxisRotationBandSpread = paramValue(X_AXIS_ROTATION_BAND_SPREAD, X_AXIS_ROTATION_BAND_SPREAD_CV, -1, 1);
  bandShiftXAxisRotatePercentage = xAxisRotationBandSpread;
  if (pinBandSpreadXsTrigger.process(params[PIN_BAND_SPREAD_XS].getValue())) {
    pinBandSpreadXs = (pinBandSpreadXs + 1) % 5;
  }
  bandShiftCells->pinXAxisValues = pinBandSpreadXs;
  bandShiftCells->pinXAxisPosition = pinXAxisPosBandSpread;
  bandShiftCells->rotateX = xAxisRotationBandSpread;
  switch (pinBandSpreadXs) {
    case 0 :
      lights[PIN_BAND_SPREAD_XS_LIGHT+0].value = 0;
      lights[PIN_BAND_SPREAD_XS_LIGHT+1].value = 0;
      lights[PIN_BAND_SPREAD_XS_LIGHT+2].value = 0;
      break;
    case 1 :
      lights[PIN_BAND_SPREAD_XS_LIGHT].value = .15;
      lights[PIN_BAND_SPREAD_XS_LIGHT+1].value = 1;
      lights[PIN_BAND_SPREAD_XS_LIGHT+2].value = .15;
      break;
    case 2 :
      lights[PIN_BAND_SPREAD_XS_LIGHT].value = 0;
      lights[PIN_BAND_SPREAD_XS_LIGHT+1].value = 1;
      lights[PIN_BAND_SPREAD_XS_LIGHT+2].value = 0;
      break;
    case 3 :
      lights[PIN_BAND_SPREAD_XS_LIGHT].value = 1;
      lights[PIN_BAND_SPREAD_XS_LIGHT+1].value = .15;
      lights[PIN_BAND_SPREAD_XS_LIGHT+2].value = .15;
      break;
    case 4 :
      lights[PIN_BAND_SPREAD_XS_LIGHT].value = 1;
      lights[PIN_BAND_SPREAD_XS_LIGHT+1].value = 0;
      lights[PIN_BAND_SPREAD_XS_LIGHT+2].value = 0;
      break;
  }

  float pinXAxisPosPanning = paramValue(X_AXIS_PIN_POS_PANNING, X_AXIS_PIN_POS_PANNING_CV, 0, 1);
  panningXAxisPercentage = pinXAxisPosPanning;
  float xAxisRotationPanning = paramValue(X_AXIS_ROTATION_PANNING, X_AXIS_ROTATION_PANNING_CV, -1, 1);
  panningXAxisRotatePercentage = xAxisRotationPanning;
  if (pinPanningXsTrigger.process(params[PIN_PANNING_XS].getValue())) {
    pinPanningXs = (pinPanningXs + 1) % 5;
  }
  panningCells->pinXAxisValues = pinPanningXs;
  panningCells->pinXAxisPosition = pinXAxisPosPanning;
  panningCells->rotateX = xAxisRotationPanning;
  switch (pinPanningXs) {
    case 0 :
      lights[PIN_PANNING_XS_LIGHT+0].value = 0;
      lights[PIN_PANNING_XS_LIGHT+1].value = 0;
      lights[PIN_PANNING_XS_LIGHT+2].value = 0;
      break;
    case 1 :
      lights[PIN_PANNING_XS_LIGHT].value = .15;
      lights[PIN_PANNING_XS_LIGHT+1].value = 1;
      lights[PIN_PANNING_XS_LIGHT+2].value = .15;
      break;
    case 2 :
      lights[PIN_PANNING_XS_LIGHT].value = 0;
      lights[PIN_PANNING_XS_LIGHT+1].value = 1;
      lights[PIN_PANNING_XS_LIGHT+2].value = 0;
      break;
    case 3 :
      lights[PIN_PANNING_XS_LIGHT].value = 1;
      lights[PIN_PANNING_XS_LIGHT+1].value = .15;
      lights[PIN_PANNING_XS_LIGHT+2].value = .15;
      break;
    case 4 :
      lights[PIN_PANNING_XS_LIGHT].value = 1;
      lights[PIN_PANNING_XS_LIGHT+1].value = 0;
      lights[PIN_PANNING_XS_LIGHT+2].value = 0;
      break;
  }


  float invertThreshold1 = paramValue(INVERT_THRESHOLD_1, INVERT_THRESHOLD_1_CV, 0, 50);
  invertThreshold1Percentage = invertThreshold1 / 50.0;
  float invertThreshold2 = paramValue(INVERT_THRESHOLD_2, INVERT_THRESHOLD_2_CV, 0, 50);
  invertThreshold2Percentage = invertThreshold2 / 50.0;


  float spread = paramValue(SPREAD, SPREAD_CV, -nbrBands, nbrBands);
  bandShiftSpreadPercentage = spread / nbrBands;


  float bandShiftX = inputs[BAND_SHIFT_X_CV].getVoltage() / 5.0;
  float bandShiftY = inputs[BAND_SHIFT_Y_CV].getVoltage() / 5.0;
  bandShiftCells->shiftX = bandShiftX;
  bandShiftCells->shiftY = bandShiftY;

  float panningShiftX = inputs[PANNING_X_CV].getVoltage() / 5.0;
  float panningShiftY = inputs[PANNING_Y_CV].getVoltage() / 5.0;
  panningCells->shiftX = panningShiftX;
  panningCells->shiftY = panningShiftY;

  float maxMagnitude1 = 0;
  float maxMagnitude2 = 0;
      
  for(uint8_t i=0; i < MAX_FRAMES; i++) {
    float windowedValue = windowFunction->windowValue(windowFunctionId,dryBuffer1[i]->setPos);
    //dryBuffer[i]->set(input * windowedValue);
    dryBuffer1[i]->set(input1 * windowedValue);
    dryBuffer2[i]->set(input2 * windowedValue);
    
    // is it time to do the fft?
    if (dryBuffer1[i]->looped) {
      curPos[i] = 0;
      fft1->fft(dryBuffer1[i]->data);
      fft2->fft(dryBuffer2[i]->data);

      uint16_t fftBand = nbrBands; 
      for(uint16_t uiBand=0;uiBand < NUM_UI_BANDS;uiBand++) {
          float uiPanning = panningCells->valueForPosition(uiBand);
          float uiBandShift = bandShiftCells->valueForPosition(uiBand)*spread;

        for (uint16_t j = 0; j < bandsPerUIBand[uiBand]; j++) {
          fftBand-=1;

          bandShift[fftBand] = uiBandShift;
          int16_t shift = fftBand + uiBandShift;
          if(shift < 0) {
            shift += nbrBands;
          } else if (shift >= nbrBands) {
            shift -= nbrBands;
          }

          panning[fftBand] = uiPanning;

          kiss_fft_cpx outValue1 = fft1->out[fftBand]; 
          float magnitude1 = sqrt(outValue1.r * outValue1.r + outValue1.i * outValue1.i);
          float phase1 = atan2(outValue1.i, outValue1.r);
          if(magnitude1 > maxMagnitude1) {
            maxMagnitude1 = magnitude1;
          }

          kiss_fft_cpx outValue2 = fft2->out[shift]; 
          float magnitude2 = sqrt(outValue2.r * outValue2.r + outValue2.i * outValue2.i);
          float phase2 = atan2(outValue2.i, outValue2.r);
          if(magnitude2 > maxMagnitude2) {
            maxMagnitude2 = magnitude2;
          }

          magnitude1Array[fftBand] = magnitude1;
          phase1Array[fftBand] = phase1;
          magnitude2Array[fftBand] = magnitude2;
          phase2Array[fftBand] = phase2;
        }
      }

      for (uint16_t j = 0; j < nbrBands; j++) {        
        float magnitude1 = invertSpectra1 && magnitude1Array[j] > invertThreshold1 ? (maxMagnitude1-magnitude1Array[j]) : magnitude1Array[j];
        float magnitude2 = invertSpectra2 && magnitude2Array[j] > invertThreshold2 ? (maxMagnitude2-magnitude2Array[j]) : magnitude2Array[j];

        float magnitude = magnitude1 * magnitude2 / 100;
        float phase = interpolate(phase1Array[j],phase2Array[j],1.0f,0.0f,1.0f);

        kiss_fft_cpx inputValue; 
        inputValue.r = magnitude*cos(phase);
        inputValue.i = magnitude*sin(phase);
        
        fft1->in[j].r = inputValue.r * std::max(1.0 - panning[j],0.0);
        fft1->in[j].i = inputValue.i * std::max(1.0 - panning[j],0.0);
        fft2->in[j].r = inputValue.r * std::max(panning[j]+1.0,1.0);
        fft2->in[j].i = inputValue.i * std::max(panning[j]+1.0,1.0);

      }      
      for(uint16_t padIndex = nbrBands; padIndex < nbrBands*2;padIndex++) {
          fft1->in[padIndex].r = 0;
          fft1->in[padIndex].i = 0;
          fft2->in[padIndex].r = 0;
          fft2->in[padIndex].i = 0;
      }
        
      fft1->ifft(processedL[i]);            
      fft2->ifft(processedR[i]);            
    }

  }

  // set the bandShift percentage
  //bandShiftPercentage = float(bandShift * 100.0) / 100.0;

  float outputL = 0;
  float outputR = 0;
  for(int i=0;i<MAX_FRAMES;i++) {
    outputL += processedL[i][curPos[i]] * windowFunction->windowValue(windowFunctionId,curPos[i]);
    outputR += processedR[i][curPos[i]] * windowFunction->windowValue(windowFunctionId,curPos[i]);
    curPos[i]++;
  }


  // float outputL = processedL[curPos];
  // float outputR = processedR[curPos];
  // curPos++;

  outputs[OUTPUT_L].setVoltage(outputL);
  outputs[OUTPUT_R].setVoltage(outputR);
}
