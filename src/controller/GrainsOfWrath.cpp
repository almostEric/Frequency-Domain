#include "GrainsOfWrath.hpp"
#include <cmath>
//#define DR_WAV_IMPLEMENTATION
#include "../model/dr_wav.h"
#include "../model/Interpolate.hpp"

using namespace frequencydomain::dsp;

GrainsOfWrathModule::GrainsOfWrathModule() {
    config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);


    // configParam(VOICE_COUNT, 1.0f, 32.0f, 32.0f,"# of Voices");
    // configParam(V_OCT_PARAM, -3.0f, 3.0f, 0.0f, "Tuning");
    // configParam(FREQ_WARP_AMOUNT, 0.0f, 1.0f, 0.0f,"Frequency Warp Amount","%",0,100);
    // configParam(FREQ_WARP_CENTER, 0.0f, 1.0f, 0.0f,"Frequency Warp Center Frequency", " Hz", 0, 10000);
    // configParam(RANDOMIZE_PARAM, 0.0f, 1.0f, 0.0f, "Randomize","%",0,100);

    configParam(START_POS_PARAM, 0.0f, 1.0f, 0.0f, "Start Position","%",0,100);
    configParam(STOP_POS_PARAM, -1.0f, 0.0f, 0.0f, "Stop Position","%",0,100);
    configParam(PLAY_SPEED_PARAM, -3.0f, 3.0f, 1.0f, "Play Speed","%",0,100);

    configParam(VOICE_WEIGHT_SCALING_PARAM, 0.0f, 1.0f, 0.0f, "Weight Scaling lin/log","%",0,100);


    configParam(WINDOW_FUNCTION_PARAM, 0.0f, 9.0f, 4.0f, "Window Function");

    configParam(GRAIN_DENSITY_PARAM, 0.1f, 1000.0f, 1.0f, "Grain Density"," grains/sec");
    configParam(GRAIN_DENSITY_VARIATION_PARAM, 0.0f, 1.0f, 0.0f, "Density Randomness","%",0,100);
    configParam(GRAIN_LENGTH_PARAM, 1.0f, 200.0f, 10.0f, "Grain Length","ms");
    configParam(GRAIN_PITCH_PARAM, -3.0f, 3.0f, 0.0f, "Grain Pitch");

    // configParam(RING_MODULATION, 0.0f, 1.0f, 0.0f, "Ring Modulation");

    for(int i=0;i<MAX_SAMPLES;i++) {
      playBuffer[i].resize(2);
      playBuffer[i][0].resize(0);
      playBuffer[i][1].resize(0); 
    }

    //grains.resize(200);

    sampleRate = APP->engine->getSampleRate();

    windowFunctionSize = sampleRate / 5.0;

    windowFunction = new WindowFunction<float>(windowFunctionSize);

    // set up one storage for matrix displays
    startPositionCells = new OneDimensionalCellsWithRollover(64, 16, 0, 1,PIN_ROLLOVER_MODE, WRAP_AROUND_ROLLOVER_MODE);
    stopPositionCells = new OneDimensionalCellsWithRollover(64, 16, 0, 1,PIN_ROLLOVER_MODE, WRAP_AROUND_ROLLOVER_MODE,1.0);
    playSpeedCells = new OneDimensionalCellsWithRollover(64, 16, -3, 3, PIN_ROLLOVER_MODE, WRAP_AROUND_ROLLOVER_MODE);
    voiceWeightCells = new OneDimensionalCellsWithRollover(32, 32, 0, 1, PIN_ROLLOVER_MODE, WRAP_AROUND_ROLLOVER_MODE);
    grainLengthCells = new OneDimensionalCellsWithRollover(64, 32, 1, 100,PIN_ROLLOVER_MODE, WRAP_AROUND_ROLLOVER_MODE);
    grainPitchCells = new OneDimensionalCellsWithRollover(64, 32, -3, 3, PIN_ROLLOVER_MODE, WRAP_AROUND_ROLLOVER_MODE);
    windowFunctionCells = new OneDimensionalCellsWithRollover(32, 32, 0, 9,PIN_ROLLOVER_MODE, WRAP_AROUND_ROLLOVER_MODE);
    panningCells = new OneDimensionalCellsWithRollover(32, 32, -1, 1, PIN_ROLLOVER_MODE, WRAP_AROUND_ROLLOVER_MODE);

    for (uint8_t i = 0; i < MAX_LIVE_INPUTS; i++) {
      liveBuffer[i] = new Buffer<float>((int)windowFunctionSize,0);
    }
    
    srand(time(NULL));

    onReset();
}

GrainsOfWrathModule::~GrainsOfWrathModule() {

  delete windowFunction;
  
  
  delete startPositionCells;
  delete stopPositionCells;
  delete playSpeedCells;
  delete voiceWeightCells;
  delete grainLengthCells;
  delete grainPitchCells;
  delete windowFunctionCells;
  delete panningCells;

  for(int i=0; i<MAX_LIVE_INPUTS;i++) {
    delete liveBuffer[i];
  }

}

void GrainsOfWrathModule::  onReset() {
    // reset the cells
    startPositionCells->reset();
    stopPositionCells->reset();
    playSpeedCells->reset();
    voiceWeightCells->reset();
    grainLengthCells->reset();
    grainPitchCells->reset();
    windowFunctionCells->reset();
    panningCells->reset();

    for(int i=0;i<MAX_SAMPLES;i++) {
      sampleStatusDesc[i] = std::to_string(i+1) + ": <Empty>";
    }

}

void GrainsOfWrathModule::dataFromJson(json_t *root) {


    json_t *ftmJ = json_object_get(root, "freezeTriggerMode");
    if (json_boolean_value(ftmJ)) {
      freezeTriggerMode = json_boolean_value(ftmJ);
    }

    json_t *pirgJ = json_object_get(root, "pitchRandomGaussian");
    if (json_boolean_value(pirgJ)) {
      pitchRandomGaussian = json_boolean_value(pirgJ);
    }

    json_t *pargJ = json_object_get(root, "panRandomGaussian");
    if (json_boolean_value(pargJ)) {
      panRandomGaussian = json_boolean_value(pargJ);
    }

    for(int i=0;i<MAX_VOICE_COUNT;i++) {
        std::string buf = "voiceWeight-" + std::to_string(i) ;
        json_t *vwJ = json_object_get(root, buf.c_str());
        if (vwJ) {
            voiceWeightCells->cells[i] = json_real_value(vwJ);
        }

        buf = "windowFunction-" + std::to_string(i) ;
        json_t *wfJ = json_object_get(root, buf.c_str());
        if (wfJ) {
            windowFunctionCells->cells[i] = json_real_value(wfJ);
        }

        buf = "grainLength-" + std::to_string(i) ;
        json_t *glJ = json_object_get(root, buf.c_str());
        if (glJ) {
            grainLengthCells->cells[i] = json_real_value(glJ);
        }

        buf = "grainPitch-" + std::to_string(i) ;
        json_t *gpJ = json_object_get(root, buf.c_str());
        if (gpJ) {
            grainPitchCells->cells[i] = json_real_value(gpJ);
        }
        buf = "grainPitchRange-" + std::to_string(i) ;
        json_t *gprJ = json_object_get(root, buf.c_str());
        if (gprJ) {
            grainPitchCells->cellExtendedValue[i] = json_real_value(gprJ);
        }

        buf = "panning-" + std::to_string(i) ;
        json_t *panJ = json_object_get(root, buf.c_str());
        if (panJ) {
            panningCells->cells[i] = json_real_value(panJ);
        }
        buf = "panningRandom-" + std::to_string(i) ;
        json_t *panrJ = json_object_get(root, buf.c_str());
        if (panrJ) {
            panningCells->cellExtendedValue[i] = json_real_value(panrJ);
        }
    }

    for(int i=0;i<MAX_SAMPLES;i++) {
        std::string buf = "lastPath-" + std::to_string(i);
        json_t *lastPathJ = json_object_get(root, buf.c_str());
        if (lastPathJ) {
            lastPath[i] = json_string_value(lastPathJ);
            reload = true ;
            loadSample(i,lastPath[i]);
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
        buf = "playSpeed-" + std::to_string(i) ;
        json_t *playSpeedJ = json_object_get(root, buf.c_str());
        if (playSpeedJ) {
            playSpeedCells->cells[i] = json_real_value(playSpeedJ);
        }
    }
}

json_t *GrainsOfWrathModule::dataToJson() {
  json_t *root = json_object();
  json_object_set(root, "freezeTriggerMode", json_boolean(freezeTriggerMode));
  json_object_set(root, "pitchRandomGaussian", json_boolean(pitchRandomGaussian));
  json_object_set(root, "panRandomGaussian", json_boolean(panRandomGaussian));

  for(int i=0;i<MAX_VOICE_COUNT;i++) {
    std::string buf = "voiceWeight-" + std::to_string(i) ;
    json_object_set(root, buf.c_str(),json_real((float) voiceWeightCells->cells[i]));

    buf = "windowFunction-" + std::to_string(i) ;
    json_object_set(root, buf.c_str(),json_real((float) windowFunctionCells->cells[i]));

    buf = "grainLength-" + std::to_string(i) ;
    json_object_set(root, buf.c_str(),json_real((float) grainLengthCells->cells[i]));

    buf = "grainPitch-" + std::to_string(i) ;
    json_object_set(root, buf.c_str(),json_real((float) grainPitchCells->cells[i]));
    buf = "grainPitchRange-" + std::to_string(i) ;
    json_object_set(root, buf.c_str(),json_real((float) grainPitchCells->cellExtendedValue[i]));

    buf = "panning-" + std::to_string(i) ;
    json_object_set(root, buf.c_str(),json_real((float) panningCells->cells[i]));
    buf = "panningRandom-" + std::to_string(i) ;
    json_object_set(root, buf.c_str(),json_real((float) panningCells->cellExtendedValue[i]));
  }

  for(int i=0;i<MAX_SAMPLES;i++) {
      std::string buf = "lastPath-" + std::to_string(i) ;
      json_object_set(root, buf.c_str(),json_string(lastPath[i].c_str()));
      buf = "startPosition-" + std::to_string(i) ;
      json_object_set(root, buf.c_str(),json_real((float) startPositionCells->cells[i]));
      buf = "stopPosition-" + std::to_string(i) ;
      json_object_set(root, buf.c_str(),json_real((float) stopPositionCells->cells[i]));
      buf = "playSpeed-" + std::to_string(i) ;
      json_object_set(root, buf.c_str(),json_real((float) playSpeedCells->cells[i]));
  }

  return root;
}

float GrainsOfWrathModule::paramValue (uint16_t param, uint16_t input, float low, float high) {
  float current = params[param].getValue();

  if (inputs[input].isConnected()) {
    // high - low, divided by one tenth input voltage, plus the current value
    current += ((inputs[input].getVoltage() / 10) * (high - low));
  }

  return clamp(current, low, high);
}


void GrainsOfWrathModule::clearSamples() {
  for(int i=0;i<MAX_SAMPLES;i++) {
    lastPath[i] = "";
    sampleStatusDesc[i] = std::to_string(i+1) + ": <Empty>";
    playBuffer[i][0].resize(0);
    playBuffer[i][1].resize(0);
    totalSampleC[i] = 0;
  }
}


void GrainsOfWrathModule::loadDirectory(std::string path) {
  DIR* rep = NULL;
  struct dirent* dirp = NULL;
  std::string dir = path.empty() ? NULL : system::getDirectory(path);

  rep = opendir(dir.c_str());
  fichier.clear();
  while ((dirp = readdir(rep)) != NULL) {
    std::string name = dirp->d_name;

    std::size_t found = name.find(".wav",name.length()-5);
    if (found==std::string::npos) found = name.find(".WAV",name.length()-5);

    if (found!=std::string::npos) {
      fichier.push_back(name);
    }
  }
  closedir(rep);
  
  for(uint64_t i=0;i<fichier.size();i++) {
      std::string fullName = dir + "/" + fichier[i];
      if(i <MAX_SAMPLES) {
        loadSample(i,fullName);
      }
  }
  
}

void GrainsOfWrathModule::loadSample(uint8_t slot, std::string path) {

    loading = true;
    fileDesc = "Loading...";
//        fprintf(stderr, "loading... \n");
    unsigned int c;
    unsigned int sr;
    drwav_uint64 tsc;
    float* pSampleData;
    pSampleData = drwav_open_and_read_file_f32(path.c_str(), &c, &sr, &tsc);

  //      fprintf(stderr, "file read \n");

	if (pSampleData != NULL) {
		channels = c;
		playerSampleRate = sr;
		playBuffer[slot][0].clear();
		playBuffer[slot][1].clear();
		for (unsigned int i=0; i < tsc; i = i + c) {
			playBuffer[slot][0].push_back(pSampleData[i]);
			if (channels == 2)
				playBuffer[slot][1].push_back((float)pSampleData[i+1]);
			
		}
		totalSampleC[slot] = playBuffer[slot][0].size();
		drwav_free(pSampleData);
        loading = false;


		fileLoaded = true;
		fileDesc = system::getFilename(path).substr(0,40);

//        fprintf(stderr, "%s \n", fileDesc.c_str());

sampleStatusDesc[slot] = std::to_string(slot+1) + ": " + fileDesc;

// 		if (reload) {
// 			
// 		}
    lastPath[slot] = path;        
	}
	else {
		
		fileLoaded = false;
	}
};


int8_t GrainsOfWrathModule::weightedProbability(float *weights,int weightCount, float scaling,float randomIn) {
  float weightTotal = 0.0f;
  float linearWeight, logWeight, weight;
          
  for(int i = 0;i < weightCount; i++) {
    linearWeight = weights[i];
    logWeight = (std::pow(10,weights[i]) - 1) / 10.0;
    weight = interpolate(linearWeight,logWeight,scaling,0.0f,1.0f);
    weightTotal += weight;
  }

  //Bail out if no weights
  if(weightTotal == 0.0)
    return 0;

  int8_t chosenWeight = -1;        
  double rnd = randomIn * weightTotal;
  for(int8_t i = 0;i < weightCount;i++ ) {
    linearWeight = weights[i];
    logWeight = (std::pow(10,weights[i]) - 1) / 10.0; 
    weight = interpolate(linearWeight,logWeight,scaling,0.0f,1.0f);
//  fprintf(stderr, "weight probablility %f  %f %f...%f %i \n",randomIn,rnd,weightTotal,weight,i);
    if(rnd <= weight && weight > 0.0) {
        chosenWeight = i;
        break;
    }
    rnd -= weight;
  }
  return chosenWeight;
}

float GrainsOfWrathModule::nextGaussian() {
  bool gaussOk = false; // don't want values that are beyond our mean
  float gaussian;
  do {
    gaussian= _gauss.next();
    gaussOk = gaussian >= -1 && gaussian <= 1;
  } while (!gaussOk);
  return gaussian;
}

void GrainsOfWrathModule::spawnGrain() {
        //fprintf(stderr, "spawning... \n");
  //uint8_t voice = 0;//TODO use weighting

  float rnd = ((float) rand()/RAND_MAX);
  int8_t voice =  weightedProbability(actualVoiceWeighting,MAX_VOICE_COUNT, weightScaling, rnd);

  if(voice < MAX_SAMPLES && totalSampleC[voice] < 1) { //Bail if nothing there
       //fprintf(stderr, "empty voice chosen... %u \n",voice);
    return;
  }
  if(voice > MAX_VOICE_COUNT || voice < 0) {
       //fprintf(stderr, "bad voice chosen... %u \n",voice);
    return;
  }
  
  uint64_t grainSize = sampleRate * (grainLength[voice] / 1000.0);
  float windowRatio = windowFunctionSize / float(grainSize);

//       fprintf(stderr, "voice chosen... %u \n",voice);

  std::vector<float> grain;
  grain.resize(grainSize);
        //fprintf(stderr, "empty grain created... \n");

  if(voice < MAX_SAMPLES) {
    float actualSamplePosition = samplePosition[voice]*(totalSampleC[voice]-1.0); //Should be play position
    for(uint64_t i=0;i<grainSize;i++) {
      float windowedValue = windowFunction->windowValue(windowFunctionId[voice],i * windowRatio);      
      grain[!reverseGrains[voice] ? i : (grainSize-1)-i] = actualSamplePosition+i < totalSampleC[voice] ? playBuffer[voice][0][actualSamplePosition+i] * windowedValue : 0.0; 
    }
  } else { // live inouts
    uint8_t liveVoice = voice - MAX_SAMPLES;
    int16_t initialLivePosition = liveBuffer[liveVoice]->setPos - grainSize + 1;
    if(initialLivePosition < 0) {
      initialLivePosition += liveBuffer[liveVoice]->size;
    }
    liveBuffer[liveVoice]->getPos = initialLivePosition;
       //fprintf(stderr, "live grain... %llu : %i %llu  \n",sampleCounter, initialLivePosition,grainSize);

       //fprintf(stderr, "live voice chosen... %u \n",liveVoice);

    for(uint64_t i=0;i<grainSize;i++) {
      float windowedValue = windowFunction->windowValue(windowFunctionId[voice],i * windowRatio);      
      float sampleValue = liveBuffer[liveVoice]->get(); 
       //fprintf(stderr, "window value... %i %f %f %f\n",i,windowedValue,sampleValue, i*windowRatio);
      grain[!reverseGrains[voice] ? i : (grainSize-1)-i] = sampleValue * windowedValue; //Scaling down live input 
    }
  }
        //fprintf(stderr, "data copied... \n");

  grains.push_back(grain);
  individualGrainVoice.push_back(voice);

        //fprintf(stderr, "grain added to queue... \n");


  individualGrainPosition.push_back(0);
  float rndPitch = (((pitchRandomGaussian ? nextGaussian() : ((float) rand()/RAND_MAX)) * 4.0) - 2.0) * grainPitchRandom[voice];
  float actualPitch = grainPitch[voice] + rndPitch;
    //    fprintf(stderr, "random / actual  pitch %f %f \n",rndPitch,actualPitch);

  float rndPan = (((panRandomGaussian ? nextGaussian() : ((float) rand()/RAND_MAX)) * 2.0) - 1.0) * panningRandom[voice];
  float pan = clamp(panning[voice] + rndPan,-1.0f,1.0f);
//        fprintf(stderr, "pan %f \n",pan);

  individualGrainPitch.push_back(actualPitch >= 0 ? actualPitch + 1.0 : 1.0 / (1.0-actualPitch));
  individualGrainReversed.push_back(reverseGrains[voice]);
  individualGrainPanning.push_back(pan);
  individualGrainSpawnTime.push_back(sampleCounter); 


  // if(reverseGrains[voice]) {
  //   fprintf(stderr, "reversed grain! \n");
  // } else {
  //   fprintf(stderr, "normal grain! \n");
  // }
        //fprintf(stderr, "individual parameters set... \n");


}


void GrainsOfWrathModule::process(const ProcessArgs &args) {
    if (freezeModeTrigger.process(params[FREEZE_TRIGGER_MODE_PARAM].getValue())) {
        freezeTriggerMode = !freezeTriggerMode;
    }
    lights[FREEZE_TRIGGER_MODE_TRIGGER_LIGHT+0].value = freezeTriggerMode ? 1.0 : 0.0;
    lights[FREEZE_TRIGGER_MODE_TRIGGER_LIGHT+1].value = freezeTriggerMode ? 1.0 : 0.0;
    lights[FREEZE_TRIGGER_MODE_TRIGGER_LIGHT+2].value = freezeTriggerMode ? 0.2 : 0.0;

    lights[FREEZE_TRIGGER_MODE_GATE_LIGHT+0].value = freezeTriggerMode ? 0.0 : 1.0;
    lights[FREEZE_TRIGGER_MODE_GATE_LIGHT+1].value = freezeTriggerMode ? 0.0 : 1.0;
    lights[FREEZE_TRIGGER_MODE_GATE_LIGHT+2].value = freezeTriggerMode ? 0.0 : 0.2;

    if (pitchGaussianTrigger.process(params[PITCH_RANDOM_GAUSSIAN_MODE_PARAM].getValue())) {
      pitchRandomGaussian = !pitchRandomGaussian;
    }
    lights[PITCH_GAUSSIAN_LIGHT+0].value = pitchRandomGaussian ? 1.0 : 0.0;
    lights[PITCH_GAUSSIAN_LIGHT+1].value = pitchRandomGaussian ? 1.0 : 0.0;
    lights[PITCH_GAUSSIAN_LIGHT+2].value = pitchRandomGaussian ? 0.2 : 0.0;

    if (panGaussianTrigger.process(params[PAN_RANDOM_GAUSSIAN_MODE_PARAM].getValue())) {
      panRandomGaussian = !panRandomGaussian;
    }
    lights[PAN_GAUSSIAN_LIGHT+0].value = panRandomGaussian ? 1.0 : 0.0;
    lights[PAN_GAUSSIAN_LIGHT+1].value = panRandomGaussian ? 1.0 : 0.0;
    lights[PAN_GAUSSIAN_LIGHT+2].value = panRandomGaussian ? 0.2 : 0.0;

    
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
    stopPositionPercentage = params[STOP_POS_PARAM].getValue()+1.0;

    float playSpeedShiftX = inputs[PLAY_SPEED_SHIFT_X_CV].getVoltage() / 5.0  + params[PLAY_SPEED_PARAM].getValue();
    float playSpeedShiftY = inputs[PLAY_SPEED_SHIFT_Y_CV].getVoltage() / 5.0;
    float playSpeedRotateX = inputs[PLAY_SPEED_ROTATE_X_CV].getVoltage() / 5.0;
    playSpeedCells->shiftX = playSpeedShiftX;
    playSpeedCells->shiftY = playSpeedShiftY;
    playSpeedCells->rotateX = playSpeedRotateX;
    playSpeedPercentage = params[PLAY_SPEED_PARAM].getValue();



    float voiceWeightShiftX = inputs[VOICE_WEIGHT_SHIFT_X_CV].getVoltage() / 5.0;
    float voiceWeightShiftY = inputs[VOICE_WEIGHT_SHIFT_Y_CV].getVoltage() / 5.0;
    float voiceWeightRotateX = inputs[VOICE_WEIGHT_ROTATE_X_CV].getVoltage() / 5.0;
    voiceWeightCells->shiftX = voiceWeightShiftX;
    voiceWeightCells->shiftY = voiceWeightShiftY;
    voiceWeightCells->rotateX = voiceWeightRotateX;

    float windowFunctionShiftX = inputs[WINDOW_FUNCTION_SHIFT_X_CV].getVoltage() / 5.0  + params[WINDOW_FUNCTION_PARAM].getValue() / 9.0;
    float windowFunctionShiftY = inputs[WINDOW_FUNCTION_SHIFT_Y_CV].getVoltage() / 5.0;
    float windowFunctionRotateX = inputs[WINDOW_FUNCTION_ROTATE_X_CV].getVoltage() / 5.0;
    windowFunctionCells->shiftX = windowFunctionShiftX;
    windowFunctionCells->shiftY = windowFunctionShiftY;
    windowFunctionCells->rotateX = windowFunctionRotateX;
    windowFunctionPercentage = params[WINDOW_FUNCTION_PARAM].getValue() / 9.0;



    float grainLengthShiftX = inputs[GRAIN_LENGTH_SHIFT_X_CV].getVoltage() / 5.0 + params[GRAIN_LENGTH_PARAM].getValue() / 200.0;
    float grainLengthShiftY = inputs[GRAIN_LENGTH_SHIFT_Y_CV].getVoltage() / 5.0;
    float grainLengthRotateX = inputs[GRAIN_LENGTH_ROTATE_X_CV].getVoltage() / 5.0;
    grainLengthCells->shiftX = grainLengthShiftX;
    grainLengthCells->shiftY = grainLengthShiftY;
    grainLengthCells->rotateX = grainLengthRotateX;
    grainLengthPercentage = params[GRAIN_LENGTH_PARAM].getValue() / 200.0;

    float grainPitchShiftX = inputs[GRAIN_PITCH_SHIFT_X_CV].getVoltage() / 5.0 + params[GRAIN_PITCH_PARAM].getValue() / 6.0;
    float grainPitchShiftY = inputs[GRAIN_PITCH_SHIFT_Y_CV].getVoltage() / 5.0;
    float grainPitchRotateX = inputs[GRAIN_PITCH_ROTATE_X_CV].getVoltage() / 5.0;
    float grainPitchRandomShiftX = inputs[GRAIN_PITCH_RANDOM_SHIFT_X_CV].getVoltage() / 5.0 + params[GRAIN_PITCH_RANDOM_PARAM].getValue();
    float grainPitchRandomShiftY = inputs[GRAIN_PITCH_RANDOM_SHIFT_Y_CV].getVoltage() / 5.0;
    float grainPitchRandomRotateX = inputs[GRAIN_PITCH_RANDOM_ROTATE_X_CV].getVoltage() / 5.0;
    grainPitchCells->shiftX = grainPitchShiftX;
    grainPitchCells->shiftY = grainPitchShiftY;
    grainPitchCells->rotateX = grainPitchRotateX;
    grainPitchCells->shiftEX = grainPitchRandomShiftX;
    grainPitchCells->shiftEY = grainPitchRandomShiftY;
    grainPitchCells->rotateEX = grainPitchRandomRotateX;
    grainPitchPercentage = params[GRAIN_PITCH_PARAM].getValue() / 3.0;

    // grainPitchRandomPercentage = params[GRAIN_PITCH_RANDOM_PARAM].getValue();


    float panShiftX = inputs[PAN_SHIFT_X_CV].getVoltage() / 5.0;
    float panShiftY = inputs[PAN_SHIFT_Y_CV].getVoltage() / 5.0;
    float panRotateX = inputs[PAN_ROTATE_X_CV].getVoltage() / 5.0;
    float panRandomShiftX = inputs[PAN_RANDOM_SHIFT_X_CV].getVoltage() / 5.0;
    float panRandomShiftY = inputs[PAN_RANDOM_SHIFT_Y_CV].getVoltage() / 5.0;
    float panRandomRotateX = inputs[PAN_RANDOM_ROTATE_X_CV].getVoltage() / 5.0;
    panningCells->shiftX = panShiftX;
    panningCells->shiftY = panShiftY;
    panningCells->rotateX = panRotateX;
    panningCells->shiftEX = panRandomShiftX;
    panningCells->shiftEY = panRandomShiftY;
    panningCells->rotateEX = panRandomRotateX;

    //get cell changes
    if(sampleCounter % 1024 == 0) {
        for (uint8_t voiceIndex = 0; voiceIndex < MAX_VOICE_COUNT; voiceIndex++) {
            voiceWeighting[voiceIndex] = voiceWeightCells->valueForPosition(voiceIndex);            
            if(voiceIndex < MAX_SAMPLES) {
              startPosition[voiceIndex]= startPositionCells->valueForPosition(voiceIndex);
              stopPosition[voiceIndex]= stopPositionCells->valueForPosition(voiceIndex);
              if(totalSampleC[voiceIndex] > 0) {
                playSpeed[voiceIndex] = playSpeedCells->valueForPosition(voiceIndex) / (totalSampleC[voiceIndex]-1.0);
                actualVoiceWeighting[voiceIndex] = voiceWeighting[voiceIndex];
                voiceWeightCells->cellColor[voiceIndex] = nvgRGB(0x3a, 0xa3, 0x27);

            //fprintf(stderr, "voice weight... %u  %f\n",voiceIndex,actualVoiceWeighting[voiceIndex]);
              } else {
                playSpeed[voiceIndex] = 0.0;
                actualVoiceWeighting[voiceIndex] = 0.0;
                voiceWeightCells->cellColor[voiceIndex] = nvgRGB(0x80, 0x00, 0x00);
              }              
            } else {
              if(inputs[LIVE_INPUT].getChannels() > voiceIndex - MAX_SAMPLES) {
                actualVoiceWeighting[voiceIndex] =  voiceWeighting[voiceIndex];
                voiceWeightCells->cellColor[voiceIndex] = liveVoiceFrozen[voiceIndex - MAX_SAMPLES] ? nvgRGB(0x40, 0x63, 0xf7) : nvgRGB(0x3a, 0xa3, 0x27);
              } else {
                actualVoiceWeighting[voiceIndex] =  0.0;
                voiceWeightCells->cellColor[voiceIndex] = nvgRGB(0x80, 0x00, 0x00);
              }              
            }

            windowFunctionId[voiceIndex] = windowFunctionCells->valueForPosition(voiceIndex);
            grainLength[voiceIndex] = grainLengthCells->valueForPosition(voiceIndex);
            grainPitch[voiceIndex] = grainPitchCells->valueForPosition(voiceIndex);
            grainPitchRandom[voiceIndex] = grainPitchCells->extendedValueForPosition(voiceIndex);
            //fprintf(stderr, "voice random... %u  %f\n",voiceIndex,grainPitchRandom[voiceIndex]);
            panning[voiceIndex] = panningCells->valueForPosition(voiceIndex);
            panningRandom[voiceIndex] = panningCells->extendedValueForPosition(voiceIndex);
        }
    }

    weightScaling = params[VOICE_WEIGHT_SCALING_PARAM].getValue();
    weightScalingPercentage = weightScaling;

    float grainDensity = paramValue(GRAIN_DENSITY_PARAM,GRAIN_DENSITY_INPUT,0.1f,1000.0f);
    float adjustment= std::max(std::sqrt(grainDensity / 50.0),1.0);
    grainDensityPercentage = grainDensity / 1000.0;
    float duration = sampleRate / grainDensity;
        
    float densityRandomness = paramValue(GRAIN_DENSITY_VARIATION_PARAM,GRAIN_DENSITY_VARIATION_INPUT,0.0f,1.0f);
    grainDensityVariationPercentage = densityRandomness;


    for(uint8_t sampleIndex =0;sampleIndex<MAX_SAMPLES;sampleIndex++) {      
      samplePosition[sampleIndex] = playSpeed[sampleIndex] != 0 ? samplePosition[sampleIndex] + playSpeed[sampleIndex] : startPosition[sampleIndex];
      if(samplePosition[sampleIndex] > stopPosition[sampleIndex]) {
        samplePosition[sampleIndex] = startPosition[sampleIndex];
      } else if (samplePosition[sampleIndex] < startPosition[sampleIndex]) {
        samplePosition[sampleIndex] = stopPosition[sampleIndex];
      }
      float reverseParamValue = inputs[REVERSE_GRAIN_SAMPLES_INPUT].getChannels() != 1 ? inputs[REVERSE_GRAIN_SAMPLES_INPUT].getVoltage(sampleIndex) : inputs[REVERSE_GRAIN_SAMPLES_INPUT].getVoltage();
      if(freezeTriggerMode) {
        if(reverseParamValue <= 0) {
          reverseGrains[sampleIndex] = false;
        } else {
          reverseGrains[sampleIndex] = true;
        }
      } else {
        if(reverseTrigger[sampleIndex].process(reverseParamValue)) {
          reverseGrains[sampleIndex] = !reverseGrains[sampleIndex];
        }
      }

    }

    for(uint8_t liveInputIndex=0; liveInputIndex < inputs[LIVE_INPUT].getChannels(); liveInputIndex++) {
      float freezeParamValue = inputs[LIVE_FREEZE_INPUT].getChannels() != 1 ? inputs[LIVE_FREEZE_INPUT].getVoltage(liveInputIndex) : inputs[LIVE_FREEZE_INPUT].getVoltage();
      float reverseParamValue = inputs[REVERSE_GRAIN_LIVE_INPUT].getChannels() != 1 ? inputs[REVERSE_GRAIN_LIVE_INPUT].getVoltage(liveInputIndex) : inputs[REVERSE_GRAIN_LIVE_INPUT].getVoltage();
      if(freezeTriggerMode) {
        if(freezeParamValue <= 0) {
          liveVoiceFrozen[liveInputIndex] = false;
        } else {
          liveVoiceFrozen[liveInputIndex] = true;
        }
        if(reverseParamValue <= 0) {
          reverseGrains[liveInputIndex+MAX_SAMPLES] = false;
        } else {
          reverseGrains[liveInputIndex+MAX_SAMPLES] = true;
        }
      } else {
        if(freezeTrigger[liveInputIndex].process(freezeParamValue)) {
          liveVoiceFrozen[liveInputIndex] = !liveVoiceFrozen[liveInputIndex];
        }
        if(reverseTrigger[liveInputIndex+MAX_SAMPLES].process(reverseParamValue)) {
          reverseGrains[liveInputIndex+MAX_SAMPLES] = !reverseGrains[liveInputIndex+MAX_SAMPLES];
        }
      }
      if(!liveVoiceFrozen[liveInputIndex]) {
        liveBuffer[liveInputIndex]->set(inputs[LIVE_INPUT].getVoltage(liveInputIndex) /adjustment ); 
      }
    }


//fprintf(stderr, "duration:%llu  %f %f\n",sampleCounter, duration,densityAddedRandomness);
    
    if((!inputs[EXTERNAL_CLOCK_INPUT].isConnected() && sampleCounter > lastGrainSampleCount + duration + densityAddedRandomness) ||
        (inputs[EXTERNAL_CLOCK_INPUT].isConnected() && playTrigger.process(inputs[EXTERNAL_CLOCK_INPUT].getVoltage()))) {
      lastGrainSampleCount = sampleCounter;
      densityAddedRandomness = ((((float) rand()/RAND_MAX) * duration - (duration / 2.0))) * densityRandomness;
      spawnGrain();
    }

    uint64_t nbrGrains = grains.size();

        //fprintf(stderr, "grain count: %llu \n",nbrGrains);
    float outL = 0.0;
    float outR = 0.0;
    for(uint64_t i=0;i<nbrGrains;i++) {
      float igp = individualGrainPosition[i];
      uint64_t grainSize = grains[i].size();
      float grainValue = grains[i][igp];

      //Add panning
      outL += grainValue * std::min(1.0 - individualGrainPanning[i],1.0);
      outR += grainValue * std::min(individualGrainPanning[i]+1.0,1.0);;

      uint8_t voice = individualGrainVoice[i];
      float lineInput = voice < MAX_SAMPLES ? inputs[V_OCT_SAMPLE_INPUT].getVoltage(voice) : inputs[V_OCT_LIVE_INPUT].getVoltage(voice-MAX_SAMPLES);
      float liveVoicePitch = lineInput >= 0 ? lineInput + 1.0 : 1.0 / (1.0-lineInput);

      individualGrainPosition[i] += (individualGrainPitch[i] * liveVoicePitch); 
      if(individualGrainPosition[i] >= grainSize) { //Remove grain
        //fprintf(stderr, "killing %llu of %llu and %llu\n",i,nbrGrains,individualGrainPanning.size());
        grains.erase(grains.begin() + i);
        individualGrainVoice.erase(individualGrainVoice.begin() + i);
        individualGrainPosition.erase(individualGrainPosition.begin() + i);
        individualGrainPitch.erase(individualGrainPitch.begin() +  i);
        individualGrainPanning.erase(individualGrainPanning.begin() +  i);
        individualGrainSpawnTime.erase(individualGrainSpawnTime.begin() +  i);
        individualGrainReversed.erase(individualGrainReversed.begin() +  i);
        i-=1;
        nbrGrains -=1;
      }
    }
   
    outputs[OUTPUT_L].setVoltage(outL);
    outputs[OUTPUT_R].setVoltage(outR);
   
    sampleCounter +=1;
}

  
//   for (uint8_t i = 0; i < 16; i++) {
//     outputs[DEBUG_OUTPUT].setVoltage(debugOutput[i], i);
//   }
//  outputs[DEBUG_OUTPUT].setChannels(16);
//}
