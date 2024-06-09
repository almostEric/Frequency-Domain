#include "BallOfConfusion.hpp"
#include <cmath>
//#define DR_WAV_IMPLEMENTATION
#include "../model/dr_wav.h"
#include "../model/Interpolate.hpp"

using namespace frequencydomain::dsp;

BallOfConfusionModule::BallOfConfusionModule() {
    config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

    sampleRate = APP->engine->getSampleRate();


    configParam(YAW_PARAM, -1.0f, 1.0f, 0.0f, "Yaw Rotation","%",0,100);
    configParam(PITCH_PARAM, -1.0f, 1.0f, 0.0f, "Pitch Rotation","%",0,100);
    configParam(ROLL_PARAM, -1.0f, 1.0f, 0.0f, "Roll Rotation","%",0,100);

    configParam(FM_AMOUNT, 0.0f, 1.0f, 0.0f, "FM Amount","%",0,100);

    configParam(FREQUENCY_PARAM, -54.f, 54.f, 0.f, "Frequency", " Hz", dsp::FREQ_SEMITONE, dsp::FREQ_C4);

  
    configParam(SYNC_POSITION_PARAM, 0.0f, 1.0f, 1.0f, "Sync Position","%",0,100);

    configParam(SPECTRUM_SHIFT_PARAM, -64.0f, 64.0f, 0.0f, "Spectrum Shift","Band(s)");

    configParam(WAVEFOLD_AMOUNT_PARAM, 1.0f, 4.0f, 1.0f, "Fold Amount");
    


    sphere.resize(0);
    fft = new FFT(WAV_TABLE_SIZE); //Might need to double suze

    binnings = new Binning(WAV_TABLE_SIZE, sampleRate);

    harmonicShiftCells = new OneDimensionalCellsWithRollover(32, 16, -4 , 4, PIN_ROLLOVER_MODE,WRAP_AROUND_ROLLOVER_MODE);

  
    srand(time(NULL));

    onReset();
}

BallOfConfusionModule::~BallOfConfusionModule() {
  delete fft;
  delete binnings;
  delete harmonicShiftCells;
}

void BallOfConfusionModule::onReset() {
  fichier.clear();
  loadFromDirectory.clear();
  lastPath.clear();

  waveTableCount = 0;
  waveTableFileCount = 0;
  waveTablePathCount = 0;

  waveTableFileSampleCount.resize(0);
  waveTableList.resize(0);
  waveTableSpectralList.resize(0);
  waveTableFundamentalHarmonicList.resize(0);
  sphere.resize(0);
}

void BallOfConfusionModule::dataFromJson(json_t *root) {

  //fprintf(stderr, "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!1   Getting JSON data \n");

    json_t *spJ = json_object_get(root, "scatterPercent");
    if (json_real_value(spJ)) {
      scatterPercent = json_real_value(spJ);
    }


    json_t *mmJ = json_object_get(root, "morphMode");
    if (json_integer_value(mmJ)) {
      morphMode = json_integer_value(mmJ);
    }

    json_t *smJ = json_object_get(root, "syncMode");
    if (json_integer_value(smJ)) {
      syncMode = json_integer_value(smJ);
    }

    json_t *wfmJ = json_object_get(root, "waveFoldMode");
    if (json_integer_value(wfmJ)) {
      waveFoldMode = json_integer_value(wfmJ);
    }

    uint16_t tempPathCount = 0;
    json_t *wtfcJ = json_object_get(root, "waveTablePathCount");
    if (json_integer_value(wtfcJ)) {
      tempPathCount = json_integer_value(wtfcJ);
    }


    for(int i=0;i<MAX_HARMONICS;i++) {
      std::string buf = "harmonicShift-" + std::to_string(i) ;
      json_t *fmmJ = json_object_get(root, buf.c_str());
      if (fmmJ) {
          harmonicShiftCells->cells[i] = json_real_value(fmmJ);
      }
    }


  for(int i=0;i<tempPathCount;i++) {
    bool tempLoadFromDirectory = false;
    std::string tempLastPath;

    std::string buf = "loadFromDirectory-" + std::to_string(i);
    json_t *lfdJ = json_object_get(root, buf.c_str());
    if (json_boolean_value(lfdJ)) {
      tempLoadFromDirectory = json_boolean_value(lfdJ);
    }

    buf = "lastPath-" + std::to_string(i) ;
    json_t *lastPathJ = json_object_get(root, buf.c_str());
    if (lastPathJ) {        
        tempLastPath = json_string_value(lastPathJ);
        if(tempLoadFromDirectory) {
          loadDirectory(tempLastPath);
        } else {
          if(i == 0) {
            loadIndividualWavefile(tempLastPath);
          } else {
            loadAdditionalWavefile(tempLastPath);
          }
        }
    }
  }

}

json_t *BallOfConfusionModule::dataToJson() {

  json_t *root = json_object();
  json_object_set(root, "scatterPercent", json_real(scatterPercent));
  json_object_set(root, "morphMode", json_integer(morphMode));
  json_object_set(root, "syncMode", json_integer(syncMode));
  json_object_set(root, "waveFoldMode", json_integer(waveFoldMode));
  json_object_set(root, "waveTablePathCount", json_integer(waveTablePathCount));

  for(int i=0;i<MAX_HARMONICS;i++) {
    std::string buf = "harmonicShift-" + std::to_string(i) ;
    json_object_set(root, buf.c_str(),json_real((float) harmonicShiftCells->cells[i]));
  }

  for(int i=0;i<waveTablePathCount;i++) {
    std::string lfd = "loadFromDirectory-" + std::to_string(i) ;
    json_object_set(root, lfd.c_str(), json_boolean(loadFromDirectory[i]));

    std::string buf = "lastPath-" + std::to_string(i) ;
    json_object_set(root, buf.c_str(),json_string(lastPath[i].c_str()));
  }


  return root;
}

float BallOfConfusionModule::paramValue (uint16_t param, uint16_t input, float low, float high) {
  float current = params[param].getValue();

  if (inputs[input].isConnected()) {
    // high - low, divided by one tenth input voltage, plus the current value
    current += ((inputs[input].getVoltage() / 10) * (high - low));
  }

  return clamp(current, low, high);
}


// void BallOfConfusionModule::clearSamples() {

// }


void BallOfConfusionModule::loadDirectory(std::string path) {
  DIR* rep = NULL;
  struct dirent* dirp = NULL;
  std::string dir = path.empty() ? NULL : system::getDirectory(path);

        //fprintf(stderr, "opening directory...%s files \n",dir.c_str());
  loading = true;
  waveTablePathCount = 1;

  rep = opendir(dir.c_str());
  fichier.clear();
  loadFromDirectory.clear();
  loadFromDirectory.push_back(true);
  lastPath.clear();
  lastPath.push_back(path);
  rebuild = true;
  while ((dirp = readdir(rep)) != NULL) {
    std::string name = dirp->d_name;

    std::size_t found = name.find(".wav",name.length()-5);
    if (found==std::string::npos) found = name.find(".WAV",name.length()-5);

    if (found!=std::string::npos) {
      fichier.push_back(name);
    }
  }
  closedir(rep);

        //fprintf(stderr, "loading...%ul files \n",fichier.size());

  waveTableFileSampleCount.clear();
  waveTableList.clear();
  waveTableSpectralList.clear();
  waveTableFundamentalHarmonicList.clear();
  waveTableNames.clear();


  waveTableCount =0;
  waveTableFileCount = 0;
  for(uint64_t i=0;i<fichier.size();i++) {
      std::string fullName = dir + "/" + fichier[i];
      loadSample(i,fullName);
  }
  waveTableFileCount = fichier.size();
  
  sphere.clear();
  fibonacci_sphere(waveTableCount);
  loading = false;
}

void BallOfConfusionModule::loadIndividualWavefile(std::string path) {
  std::string dir = path.empty() ? NULL : system::getDirectory(path);

//fprintf(stderr, "opening individual file...%s \n",path.c_str());
  loading = true;
  
  fichier.clear();
  waveTableFileSampleCount.clear();
  waveTableList.clear();
  waveTableSpectralList.clear();
  waveTableFundamentalHarmonicList.clear();
  waveTableNames.clear();

  waveTableCount =0;
  waveTableFileCount = 0;
  waveTablePathCount = 0;
  loadFromDirectory.clear();
  lastPath.clear();
  
  loadAdditionalWavefile(path);
}

void BallOfConfusionModule::loadAdditionalWavefile(std::string path) {
  std::string dir = path.empty() ? NULL : system::getDirectory(path);

  loading = true;
  rebuild = true;

//fprintf(stderr, "opening additional file...%s, %i \n",path.c_str(), waveTablePathCount);

  std::string fullName = path;
  fichier.push_back(fullName.substr(dir.length()+1));
  loadFromDirectory.push_back(false);
  lastPath.push_back(path);
  loadSample(waveTableFileCount,fullName);
  waveTableFileCount +=1;
  waveTablePathCount +=1;
  
  sphere.clear();
  fibonacci_sphere(waveTableCount);
  
  loading = false;
  
}


void BallOfConfusionModule::loadSample(uint8_t slot, std::string path) {

    loading = true;
    fileDesc = "Loading...";
//        fprintf(stderr, "loading... \n");
    unsigned int c;
    unsigned int sr;
    drwav_uint64 tsc;
    float* pSampleData;
    pSampleData = drwav_open_and_read_file_f32(path.c_str(), &c, &sr, &tsc);

        //fprintf(stderr, "file read - size %ul \n",tsc);

	if (pSampleData != NULL && tsc % WAV_TABLE_SIZE == 0) {
    uint16_t sampleCount = tsc/WAV_TABLE_SIZE; 
    waveTableCount+= sampleCount;
    uint16_t fftIndex = 0;
    uint16_t sampleIndex = 1;
    waveTableFileSampleCount.push_back(waveTableCount);
		for (unsigned int i=0; i < tsc; i = i + c) {
			waveTableList.push_back(pSampleData[i]);
      fftBuffer[fftIndex] = pSampleData[i];
      fftIndex++;
      if(fftIndex == WAV_TABLE_SIZE) {
        std::string fullName = fichier[slot];        
        int nameLength = fullName.length()-4; 
        waveTableNames.push_back(fullName.substr(0,std::min(nameLength,40)) + " / " + std::to_string(sampleIndex));
        sampleIndex++;
        fftIndex = 0;
        fft->fft(fftBuffer);
        for (uint16_t j=0; j < WAV_TABLE_SIZE / 2; j++) {
          kiss_fft_cpx outValue = fft->out[j]; 
          float magnitude = std::sqrt(outValue.r * outValue.r + outValue.i * outValue.i);
          float phase = std::atan2(outValue.i, outValue.r);
          waveTableSpectralList.push_back(magnitude);
          waveTableSpectralList.push_back(phase);
          magnitudeBuffer[j] = magnitude;
          phaseBuffer[j] = phase;
        }
        binnings->topN(1, magnitudeBuffer, phaseBuffer, bins, (FFTSortMode) 0);
        waveTableFundamentalHarmonicList.push_back(bins[0].binNumber);

      }
		}
		drwav_free(pSampleData);
    loading = false;


		fileLoaded = true;
		fileDesc = system::getFilename(path).substr(0,40);

        //fprintf(stderr, "%s \n", fileDesc.c_str());

	}
};


void BallOfConfusionModule::fibonacci_sphere(uint16_t samples) {
    if(samples == 0)
      return;

    samples -=1; // decrement for scaling

    float phi = M_PI * (3.0 - std::sqrt(5.0));  // golden angle in radians

    uint16_t scatter = clamp(scatterPercent * samples,1.0,samples-1.0);

//fprintf(stderr, "s:%u sp:%f  \n",scatter,scatterPercent);

    uint16_t pointIndex = 0;
    uint16_t fileIndex = 0;
    for(uint16_t scatterIndex = 0;scatterIndex<scatter;scatterIndex++) {
      for(uint16_t i=scatterIndex;i<=samples;i+=scatter) {
          //uint16_t actualI = (i+scatterIndex) % samples;
          float y = 1 - (i / float(samples)) * 2;  // y goes from 1 to -1
          float radius = std::sqrt(1 - y * y);  // radius at y

          float theta = phi * i;  // golden angle increment

          float x = std::cos(theta) * radius;
          float z = std::sin(theta) * radius;

          point3d *point = new point3d(fileIndex,pointIndex,x,y,z);
          pointIndex++;
          if(pointIndex >= waveTableFileSampleCount[fileIndex]) {
            fileIndex++;
          }


          //fprintf(stderr, "i:%i x:%f y:%f z:%f \n",i,x,y,z);
          sphere.push_back(*point);        
      }
    }
}

void BallOfConfusionModule::rotateSphere(float yaw,float pitch,float roll) {
  float rotationMatrix[3][3];

  yaw *= M_PI;
  pitch *= M_PI_2;
  roll *= M_PI;

  rotationMatrix[0][0] = std::cos(yaw) * std::cos(pitch);
  rotationMatrix[0][1] = std::cos(yaw) * std::sin(pitch) * std::sin(roll) - std::sin(yaw) * std::cos(roll);
  rotationMatrix[0][2] = std::cos(yaw) * std::sin(pitch) * std::cos(roll) + std::sin(yaw) * std::sin(roll);
  rotationMatrix[1][0] = std::sin(yaw) * std::cos(pitch);
  rotationMatrix[1][1] = std::sin(yaw) * std::sin(pitch) * std::sin(roll) + std::cos(yaw) * std::cos(roll);
  rotationMatrix[1][2] = std::sin(yaw) * std::sin(pitch) * std::cos(roll) - std::cos(yaw) * std::sin(roll);
  rotationMatrix[2][0] = -std::sin(pitch);
  rotationMatrix[2][1] = std::cos(pitch) * std::sin(roll);
  rotationMatrix[2][2] = std::cos(pitch) * std::cos(roll);


  for (uint16_t c = 0; c < MAX_MORPHED_WAVETABLES; c++) {
    waveTableDistance[c] = 100;
    waveTablesInUse[c] = 0;
  }

  float xOrigin = 0.0;
  float yOrigin = -std::pow(sphere.size(),1.0/3.0) / 100.0;
    //fprintf(stderr, "%f \n",yOrigin);
  //float yOrigin = 0.0;
  float zOrigin = -1.0;


  for(uint32_t s=0;s<sphere.size();s++) {
    sphere[s].xRotated = rotationMatrix[0][0] * sphere[s].x + rotationMatrix[0][1] * sphere[s].y + rotationMatrix[0][2] * sphere[s].z;
    sphere[s].yRotated = rotationMatrix[1][0] * sphere[s].x + rotationMatrix[1][1] * sphere[s].y + rotationMatrix[1][2] * sphere[s].z;
    sphere[s].zRotated = rotationMatrix[2][0] * sphere[s].x + rotationMatrix[2][1] * sphere[s].y + rotationMatrix[2][2] * sphere[s].z;

    float distance = (sphere[s].xRotated-xOrigin) * (sphere[s].xRotated-xOrigin) + (sphere[s].yRotated-yOrigin) * (sphere[s].yRotated-yOrigin) + (sphere[s].zRotated - zOrigin) * (sphere[s].zRotated - zOrigin);

    for (int16_t c = 0; c < MAX_MORPHED_WAVETABLES; c++) {
      if(waveTableDistance[c] >= distance) {
        for (uint16_t mc = MAX_MORPHED_WAVETABLES-1; mc > c ; mc--) {
          waveTablesInUse[mc] = waveTablesInUse[mc-1];
          waveTableDistance[mc] = waveTableDistance[mc-1];
        }
        waveTablesInUse[c] = s;
        waveTableDistance[c] = distance;
        break;
      }
    }
  }
}


void BallOfConfusionModule::buildActualWaveTable() {

  uint16_t fundamentalBin = 1;

  if(morphMode < MORPH_TRANSFER) {
    float totalWeight = (waveTableDistance[0] * waveTableDistance[1] * waveTableDistance[2]) + 
                          (waveTableDistance[1] * waveTableDistance[2] * waveTableDistance[3]) + 
                          (waveTableDistance[0] * waveTableDistance[2] * waveTableDistance[3]) + 
                          (waveTableDistance[0] * waveTableDistance[1] * waveTableDistance[3]);

    waveTableWeighting[0] = (waveTableDistance[1] * waveTableDistance[2] * waveTableDistance[3] / totalWeight);
    waveTableWeighting[1] = (waveTableDistance[0] * waveTableDistance[2] * waveTableDistance[3] / totalWeight);
    waveTableWeighting[2] = (waveTableDistance[0] * waveTableDistance[1] * waveTableDistance[3] / totalWeight);
    waveTableWeighting[3] = (waveTableDistance[0] * waveTableDistance[1] * waveTableDistance[2] / totalWeight);

    for(int c=0;c<4;c++) {
      fundamentalBin += waveTableFundamentalHarmonicList[waveTablesInUse[c]] * waveTableWeighting[c];
    }


  } else {
    float totalWeight = (waveTableDistance[1] * waveTableDistance[2]) + 
                          (waveTableDistance[2] * waveTableDistance[3]) + 
                          (waveTableDistance[1] * waveTableDistance[3]);

    waveTableWeighting[0] = 1.0;
    waveTableWeighting[1] = (waveTableDistance[2] * waveTableDistance[3] / totalWeight);
    waveTableWeighting[2] = (waveTableDistance[1] * waveTableDistance[3] / totalWeight);
    waveTableWeighting[3] = (waveTableDistance[1] * waveTableDistance[2] / totalWeight);
  }



  uint16_t j = 0;
  for(int i=0;i<WAV_TABLE_SIZE;i++) {
    if(morphMode == MORPH_INTERPOLATE) { 
      float interpolatedValue = 0;
      for (uint16_t c = 0; c < MAX_MORPHED_WAVETABLES; c++) {
        int16_t adjustedI = (i + spectrumShift*c*4) % (WAV_TABLE_SIZE);
        if(adjustedI < 0)
          adjustedI += (WAV_TABLE_SIZE);
        interpolatedValue +=waveTableList[waveTablesInUse[c]*WAV_TABLE_SIZE+adjustedI] * waveTableWeighting[c];
      }      
      prefoldedWaveTable[i] = interpolatedValue;
    } else if (morphMode == MORPH_SPECTRAL || morphMode == MORPH_SPECTRAL_0_PHASE) { 
      if(i % 2 == 1) {

        float interpolatedMagnitude = 0;
        float interpolatedPhase = 0;
        for (uint16_t c = 0; c < MAX_MORPHED_WAVETABLES; c++) {
          int16_t adjustedI = (i + spectrumShift*c*2) % (WAV_TABLE_SIZE);
          if(adjustedI < 0)
            adjustedI += (WAV_TABLE_SIZE);
  
          interpolatedMagnitude +=waveTableSpectralList[waveTablesInUse[c]*WAV_TABLE_SIZE+adjustedI-1] * waveTableWeighting[c];
          if(morphMode == 1) {
            interpolatedPhase +=waveTableSpectralList[waveTablesInUse[c]*WAV_TABLE_SIZE+adjustedI] * waveTableWeighting[c];          
          }
        }
        if(j % fundamentalBin == 0) {
          uint16_t harmonic = j / fundamentalBin;
          if(harmonic < 16) {
            interpolatedMagnitude *= harmonicShiftAmount[harmonic];
          }
        }
        fft->in[j].r = interpolatedMagnitude*cos(interpolatedPhase);
        fft->in[j].i = interpolatedMagnitude*sin(interpolatedPhase);
        j++;
      }
    } else { //transfer function
      float interpolatedValue = 0;
      for (uint16_t c = 1; c < MAX_MORPHED_WAVETABLES; c++) {
        int16_t adjustedI = (i + spectrumShift*c*4) % (WAV_TABLE_SIZE);
        if(adjustedI < 0)
          adjustedI += (WAV_TABLE_SIZE);
        interpolatedValue +=waveTableList[waveTablesInUse[c]*WAV_TABLE_SIZE+adjustedI] * waveTableWeighting[c];
      }
      interpolatedValue = clamp((interpolatedValue+1) * (WAV_TABLE_SIZE / 2),0.0,WAV_TABLE_SIZE-1.0);      
      // fprintf(stderr, "!!!! transfer interpolated point %f \n",interpolatedValue);
      prefoldedWaveTable[i] =  waveTableList[waveTablesInUse[0]*WAV_TABLE_SIZE+interpolatedValue]; // i for now - should be interpolated value after it is scaled

    }


  }
  if (morphMode == MORPH_SPECTRAL || morphMode == MORPH_SPECTRAL_0_PHASE) { //Inverse FFT
    for(uint16_t j = WAV_TABLE_SIZE / 2; j < WAV_TABLE_SIZE;j++) {
        fft->in[j].r = 0;
        fft->in[j].i = 0; 
    }
    fft->ifft(ifftWaveTable);
    for(uint16_t j =0;j<WAV_TABLE_SIZE;j++) {
      prefoldedWaveTable[j] = clamp(ifftWaveTable[j],-1.0,1.0);
    }
  }
}

void BallOfConfusionModule::calculateWaveFolding() {

  float decimation = 1024 / pow(4,wavefoldAmount*1.25);

  for(int i=0;i<WAV_TABLE_SIZE;i++) {

    float foldedValue = (prefoldedWaveTable[i]);
    
    if(waveFoldMode == WAVEFOLD_INVERT) {
      foldedValue = (prefoldedWaveTable[i]) * wavefoldAmount;
      while(foldedValue < -1.0 || foldedValue > 1.0) {
        if(foldedValue < -1.0) {
          foldedValue = -1.0-foldedValue ;
        }
        if(foldedValue > 1.0) {
          foldedValue = 1.0 - foldedValue;
        }
      }
    } else { // DECIMATE
      foldedValue = round(prefoldedWaveTable[i] * decimation) / decimation;
    }

    actualWaveTable[i] = foldedValue;
  }
}

void BallOfConfusionModule::process(const ProcessArgs &args) {
    
  yaw = paramValue(YAW_PARAM,YAW_INPUT,-1.0f,1.0f);
  yawPercentage = yaw;
  pitch = paramValue(PITCH_PARAM,PITCH_INPUT,-1.0f,1.0f);
  pitchPercentage = pitch;
  roll = paramValue(ROLL_PARAM,ROLL_INPUT,-1.0f,1.0f);
  rollPercentage = roll;


  spectrumShift = paramValue(SPECTRUM_SHIFT_PARAM,SPECTRUM_SHIFT_INPUT,-64.0f,64.0f);
  spectrumShiftPercentage = spectrumShift / 64.0;

  wavefoldAmount = paramValue(WAVEFOLD_AMOUNT_PARAM,WAVEFOLD_AMOUNT_INPUT,1.0f,4.0f);
  wavefoldAmountPercentage = (wavefoldAmount-1.0) / 3.0;

  float harmonicShiftX = inputs[HARMONIC_SHIFT_X_INPUT].getVoltage() / 5.0;
  float harmonicShiftY = inputs[HARMONIC_SHIFT_Y_INPUT].getVoltage() / 5.0;
  harmonicShiftCells->shiftY = harmonicShiftX; //Axis are reversed
  harmonicShiftCells->shiftX = harmonicShiftY;


  const float epsilon = .001;


  if (morphModeTrigger.process(params[MORPH_MODE_PARAM].getValue())) {
    morphMode = (morphMode + 1) % NBR_MORPH_MODES; //Will have more
  }
  switch(morphMode) {
    case MORPH_INTERPOLATE : 
    lights[MORPH_MODE_LIGHT+0].value = 0;
    lights[MORPH_MODE_LIGHT+1].value = 0;
    lights[MORPH_MODE_LIGHT+2].value = 0.0;
    break;
    case MORPH_SPECTRAL : 
    lights[MORPH_MODE_LIGHT+0].value = 1;
    lights[MORPH_MODE_LIGHT+1].value = 1;
    lights[MORPH_MODE_LIGHT+2].value = 0.2;
    break;
    case MORPH_SPECTRAL_0_PHASE : 
    lights[MORPH_MODE_LIGHT+0].value = .2;
    lights[MORPH_MODE_LIGHT+1].value = 1;
    lights[MORPH_MODE_LIGHT+2].value = 1;
    break;
    case MORPH_TRANSFER : 
    lights[MORPH_MODE_LIGHT+0].value = 0.8;
    lights[MORPH_MODE_LIGHT+1].value = 0.4;
    lights[MORPH_MODE_LIGHT+2].value = 0.05;
    break;
  }

  if (syncModeTrigger.process(params[SYNC_MODE_PARAM].getValue())) {
    syncMode = (syncMode + 1) % NBR_SYNC_MODES; 
  }
  switch(syncMode) {
    case 0 : //HARD
    lights[SYNC_MODE_LIGHT+0].value = 0;
    lights[SYNC_MODE_LIGHT+1].value = 0;
    lights[SYNC_MODE_LIGHT+2].value = 0.0;
    break;
    case 1 : //SOFT
    lights[SYNC_MODE_LIGHT+0].value = 1;
    lights[SYNC_MODE_LIGHT+1].value = 1;
    lights[SYNC_MODE_LIGHT+2].value = 0.2;
    break;
    case 2 : //SOFT REVERSE
    lights[SYNC_MODE_LIGHT+0].value = .2;
    lights[SYNC_MODE_LIGHT+1].value = 1;
    lights[SYNC_MODE_LIGHT+2].value = 1;
    break;
  }

  if (wavefoldModeTrigger.process(params[WAVEFOLD_MODE_PARAM].getValue())) {
    waveFoldMode = (waveFoldMode + 1) % NBR_WAVEFOLD_MODES; 
  }
  switch(waveFoldMode) {
    case WAVEFOLD_INVERT : 
      lights[WAVEFOLD_MODE_LIGHT+0].value = 1;
      lights[WAVEFOLD_MODE_LIGHT+1].value = 1;
      lights[WAVEFOLD_MODE_LIGHT+2].value = 0.2;
      break;
    case WAVEFOLD_DECIMATE :
      lights[WAVEFOLD_MODE_LIGHT+0].value = .2;
      lights[WAVEFOLD_MODE_LIGHT+1].value = 1;
      lights[WAVEFOLD_MODE_LIGHT+2].value = 1;
      break;
  }


  if(lastScatterPercent != scatterPercent) {
    loading = true;
    sphere.clear();
    fibonacci_sphere(waveTableCount);
    lastScatterPercent = scatterPercent;
    loading = false;
  }

  //get cell changes
  if(samplesPlayed % 1024 == 0) {
      for (uint8_t harmonicIndex = 0; harmonicIndex < 16; harmonicIndex++) {
          harmonicShiftAmount[harmonicIndex] = std::pow(2,harmonicShiftCells->valueForPosition(harmonicIndex));
          if(lastHarmonicShiftAmount[harmonicIndex] != harmonicShiftAmount[harmonicIndex]) {
            recalculateWave = true;
            lastHarmonicShiftAmount[harmonicIndex] = harmonicShiftAmount[harmonicIndex];
          }
      }
      samplesPlayed =0;
  }
  samplesPlayed ++;


  if(sphere.size() > 0 && !loading) {
    if(rebuild || std::abs(yaw-lastYaw) > epsilon || std::abs(pitch-lastPitch) > epsilon || std::abs(roll-lastRoll) > epsilon) {
      rotateSphere(yaw,pitch,roll);
      lastYaw = yaw;
      lastPitch = pitch;
      lastRoll = roll;
      rebuild = false;
      recalculateWave = true;
    }

    if(recalculateWave || morphMode != lastMorphMode || lastSpectrumShift != spectrumShift) {
      buildActualWaveTable();
      lastMorphMode = morphMode;
      lastSpectrumShift = spectrumShift;
      recalculateWave = false;
      recalcFold = true;
    }

    if(recalcFold || waveFoldMode != lastWaveFoldMode || std::abs(wavefoldAmount-lastWavefoldAmount) > epsilon) {
      calculateWaveFolding();
      recalcFold = false;
      lastWavefoldAmount = wavefoldAmount;
      lastWaveFoldMode = waveFoldMode;
    }
  }


  float freqParam = params[FREQUENCY_PARAM].getValue() / 12.f;
  tuningPercentage = (freqParam / 9.0) + 0.5;
  //float fmParam = dsp::quadraticBipolar(params[FM_PARAM].getValue());
  float fmParam = paramValue(FM_AMOUNT,FM_AMOUNT_INPUT,0.0f,1.0f);;
  fmAmountPercentage = fmParam;

  syncPosition = paramValue(SYNC_POSITION_PARAM,SYNC_POSITION_INPUT,0.0f,1.0f);
  syncPositionPercentage = syncPosition;

  int channels = std::max(inputs[V_OCTAVE_INPUT].getChannels(), 1);

  int channelIndex = 0;
  for (int c = 0; c < channels; c += 4) {
    auto* oscillator = &oscillators[c / 4];
    oscillator->channels = std::min(channels - c, 4);
    oscillator->soft = syncMode > 0;
    oscillator->softReverse = syncMode == 2;
    oscillator->setSoftSyncPhase(syncPosition);

    float_4 pitch = freqParam;
    pitch += inputs[V_OCTAVE_INPUT].getVoltageSimd<float_4>(c);
    if (inputs[FM_INPUT].isConnected()) {
      pitch += fmParam * inputs[FM_INPUT].getPolyVoltageSimd<float_4>(c);
    }
    oscillator->setPitch(pitch);
    if(inputs[PHASE_INPUT].isConnected()) {
      float_4 phase = simd::clamp(inputs[PHASE_INPUT].getPolyVoltageSimd<float_4>(c)/5.0,0,1);
      oscillator->setBasePhase(phase);
    }

    oscillator->syncEnabled = inputs[SYNC_INPUT].isConnected();
    oscillator->process(args.sampleTime, inputs[SYNC_INPUT].getPolyVoltageSimd<float_4>(c));

    // Set output
    for(int i=0;i<4;i++) {
      float tableIndex = oscillator->wt()[i];
      int mainIndex = std::floor(tableIndex);
      int nextIndex = (mainIndex + 1) % WAV_TABLE_SIZE;
      float mainValue = actualWaveTable[mainIndex];
      float nextValue = actualWaveTable[nextIndex];
      float balance = tableIndex - mainIndex;
      float interpolatedValue = interpolate(mainValue,nextValue,balance,0.0f,1.0f);
      
      outputs[OUTPUT_L].setVoltage(5.f * interpolatedValue, channelIndex);
      channelIndex++;
    }
  }

  outputs[OUTPUT_L].setChannels(channels);
      
}
