#include "DelayedReaction.hpp"

#include <vector>
#include <cmath>

DelayedReactionModule::DelayedReactionModule() {
  config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

  configParam(MIX, 0.0f, 1.0f, 0.5f, "Mix","%",0,100);


  configParam(X_AXIS_PIN_POS_ATTENUATION, 0.0f, 1.0f, 0.0f, "Attenuation X Axis Pin Position","%",0,100);
  configParam(X_AXIS_PIN_POS_DELAY_TIME, 0.0f, 1.0f, 0.0f, "Delay Time X Axis Pin Position","%",0,100);
  configParam(X_AXIS_PIN_POS_FEEDBACK, 0.0f, 1.0f, 0.0f, "Feedback X Axis Pin Position","%",0,100);
  configParam(X_AXIS_PIN_POS_PANNING, 0.0f, 1.0f, 0.0f, "Panning X Axis Pin Position","%",0,100);

  configParam(X_AXIS_ROTATION_ATTENUATION, -1.0f, 1.0f, 0.0f, "Attenuation X Axis Rotation","°",0,180);
  configParam(X_AXIS_ROTATION_DELAY_TIME, -1.0f, 1.0f, 0.0f, "Delay Time X Axis Rotation","°",0,180);
  configParam(X_AXIS_ROTATION_FEEDBACK, -1.0f, 1.0f, 0.0f, "Feedback X Axis Rotation","°",0,180);
  configParam(X_AXIS_ROTATION_PANNING, -1.0f, 1.0f, 0.0f, "Panning X Axis Rotation","°",0,180);

  configButton(PIN_ATTENUATION_0S,"Attenuation Grid Pin X Axis Mode");
  configButton(PIN_DELAY_TIME_0S,"Delay Time Grid Pin X Axis Mode");
  configButton(PIN_FEEDBACK_0S,"Feedback Grid Pin X Axis Mode");
  configButton(PIN_PANNING_0S,"Panning Grid Pin X Axis Mode");

  configButton(LINK_ATTENUATION,"Link Attention Grid to 2nd Delayed Reaction");
  configButton(LINK_DELAY_TIME,"Link Delay Time Grid to 2nd Delayed Reaction");
  configButton(LINK_FEEDBACK,"Link Feedback Grid to 2nd Delayed Reaction");

  configButton(DELAY_RANGE,"Delay Time Range");


  configInput(INPUT, "Main");
  configInput(MIX_CV, "Mix CV");
  configInput(FEEDBACK_RETURN, "Feedback Return");

  configInput(ATTENUATION_X_CV, "Attenuation Grid X Shift CV");
  configInput(ATTENUATION_Y_CV, "Attenuation Grid Y Shift CV");
  configInput(X_AXIS_PIN_POS_ATTENUATION_CV, "Attenuation Grid X Axis Pin CV");
  configInput(X_AXIS_ROTATION_ATTENUATION_CV, "Attenuation Grid X Axis Rotation CV"); 

  configInput(DELAY_TIME_X_CV, "Delay Time Grid X Shift CV");
  configInput(DELAY_TIME_Y_CV, "Delay Time Grid Y Shift CV");
  configInput(X_AXIS_PIN_POS_DELAY_TIME_CV, "Delay Time Grid X Axis Pin CV");
  configInput(X_AXIS_ROTATION_DELAY_CV, "Delay Time Grid X Axis Rotation CV");

  configInput(DELAY_FEEDBACK_X_CV, "Feedback Grid X Shift CV");
  configInput(DELAY_FEEDBACK_Y_CV, "Feedback Grid Y Shift CV");
  configInput(X_AXIS_PIN_POS_FEEDBACK_CV, "Feedback Grid X Axis Pin CV");
  configInput(X_AXIS_ROTATION_FEEDBACK_CV, "Feedback Grid X Axis Rotation CV");

  configInput(PANNING_X_CV, "Panning Grid X Shift CV");
  configInput(PANNING_Y_CV, "Panning Grid Y Shift CV");
  configInput(X_AXIS_PIN_POS_PANNING_CV, "Panning Grid X Axis Pin CV");
  configInput(X_AXIS_ROTATION_PANNING_CV, "Panning Grid X Axis Rotation CV");

  configOutput(OUTPUT_L, "Left/Mono");
  configOutput(OUTPUT_L, "Right");
  configOutput(FEEDBACK_SEND, "Feedback Send");



  leftExpander.producerMessage = producerMessage;
  leftExpander.consumerMessage = consumerMessage;
  
  frameSize = 11;
  nbrBands = pow(2.0,frameSize-1);
  lastFrameSize = frameSize;
  hopSize = frameSize - 2;
  baseDelaySizeinBytes = 163840; // * 10 so it can get rounded to .1

  delayAdjustment = pow(2,frameSize-11); //keep delay time constant, independent of frame sizze

  sampleRate = APP->engine->getSampleRate();
  currentBin = 0;

  fft = new FFT(nbrBands * 2);
  feedbackfft = new FFT(nbrBands * 2);

  windowFunction = new WindowFunction<float>(pow(2.0f, frameSize));
  dry = new Buffer<float>(pow(2.0f, frameSize), 0);

  for (uint8_t i = 0; i < MAX_FRAMES; i++) {
    dryBuffer[i] = new Buffer<float>(pow(2.0f, frameSize),i * pow(2.0f,hopSize));

    feedbackResult[i] = new float[nbrBands*2] { 0 };
    processed[i] = new float[nbrBands*2] { 0 };

    curPos[i] = i * pow(2.0f,hopSize);
  } 

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


  attenuationCells = new OneDimensionalCellsWithRollover(50, NUM_UI_BANDS, 0, 1, PIN_ROLLOVER_MODE, WRAP_AROUND_ROLLOVER_MODE,1.0);
  delayTimeCells = new OneDimensionalCellsWithRollover(50, NUM_UI_BANDS, 0, 1, PIN_ROLLOVER_MODE, WRAP_AROUND_ROLLOVER_MODE);
  feedbackCells = new OneDimensionalCellsWithRollover(50, NUM_UI_BANDS, 0, 1, PIN_ROLLOVER_MODE, WRAP_AROUND_ROLLOVER_MODE);
  panningCells = new OneDimensionalCellsWithRollover(50, NUM_UI_BANDS, -1, 1, PIN_ROLLOVER_MODE, WRAP_AROUND_ROLLOVER_MODE,0.0);

  onReset();
}

DelayedReactionModule::~DelayedReactionModule() {
  delete windowFunction;
  delete dry;
  for (uint8_t i = 0; i < MAX_FRAMES; i++) {
    if (dryBuffer[i]) {
      delete dryBuffer[i];
    }
  }
}


void DelayedReactionModule::dataFromJson(json_t *root) {


  json_t *wfJ = json_object_get(root, "windowFunction");
  if (json_is_integer(wfJ)) {
    windowFunctionId = (uint8_t) json_integer_value(wfJ);
  }

  json_t *fsJ = json_object_get(root, "frameSize");
  if (json_is_integer(fsJ)) {
    frameSize = (uint8_t) json_integer_value(fsJ);
  }

  json_t *drJ = json_object_get(root, "delayRange");
  if (json_is_integer(drJ)) {
    delayRange = (uint8_t) json_integer_value(drJ);
  }

  json_t *a0J = json_object_get(root, "pinAttenuation0s");
  if (json_is_integer(a0J)) {
    pinAttenuation0s = (uint8_t) json_integer_value(a0J);
  }

  json_t *dt0J = json_object_get(root, "pinDelayTime0s");
  if (json_is_integer(dt0J)) {
    pinDelayTime0s = (uint8_t) json_integer_value(dt0J);
  }

  json_t *fb0J = json_object_get(root, "pinFeedback0s");
  if (json_is_integer(fb0J)) {
    pinFeedback0s = (uint8_t) json_integer_value(fb0J);
  }

  json_t *pn0J = json_object_get(root, "pinPanning0s");
  if (json_is_integer(pn0J)) {
    pinPanning0s = (uint8_t) json_integer_value(pn0J);
  }

  json_t *alJ = json_object_get(root, "attenuationLinked");
  if (json_is_integer(alJ)) {
    attenuationLinked = (bool) json_integer_value(alJ);
  }

  json_t *dtlJ = json_object_get(root, "delayTimeLinked");
  if (json_is_integer(dtlJ)) {
    delayTimeLinked = (bool) json_integer_value(dtlJ);
  }

  json_t *fblJ = json_object_get(root, "feedbackLinked");
  if (json_is_integer(fblJ)) {
    feedbackLinked = (bool) json_integer_value(fblJ);
  }
  
  for(int i=0;i<NUM_UI_BANDS;i++) {
    std::string buf = "attenuation-" + std::to_string(i) ;
    json_t *attJ = json_object_get(root, buf.c_str());
    if (attJ) {
      attenuationCells->cells[i] = json_real_value(attJ);
    }
    buf = "delayTime-" + std::to_string(i) ;
    json_t *dtJ = json_object_get(root, buf.c_str());
    if (dtJ) {
      delayTimeCells->cells[i] = json_real_value(dtJ);
    }
    buf = "feedback-" + std::to_string(i) ;
    json_t *fbJ = json_object_get(root, buf.c_str());
    if (fbJ) {
      feedbackCells->cells[i] = json_real_value(fbJ);
    }
    buf = "panning-" + std::to_string(i) ;
    json_t *pnJ = json_object_get(root, buf.c_str());
    if (pnJ) {
      panningCells->cells[i] = json_real_value(pnJ);
    }
  }
}

json_t *DelayedReactionModule::dataToJson() {
  json_t *root = json_object();

  json_object_set(root, "windowFunction", json_integer(windowFunctionId));
  json_object_set(root, "frameSize", json_integer(frameSize));
  
  json_object_set(root, "delayRange", json_integer(delayRange));
  json_object_set(root, "pinAttenuation0s", json_integer(pinAttenuation0s));
  json_object_set(root, "pinDelayTime0s", json_integer(pinDelayTime0s));
  json_object_set(root, "pinFeedback0s", json_integer(pinFeedback0s));
  json_object_set(root, "pinPanning0s", json_integer(pinPanning0s));
  json_object_set(root, "attenuationLinked", json_integer(attenuationLinked));
  json_object_set(root, "delayTimeLinked", json_integer(delayTimeLinked));
  json_object_set(root, "feedbackLinked", json_integer(feedbackLinked));
  for(int i=0;i<NUM_UI_BANDS;i++) {
    std::string buf = "attenuation-" + std::to_string(i) ;
    json_object_set(root, buf.c_str(),json_real((float) attenuationCells->cells[i]));
    buf = "delayTime-" + std::to_string(i) ;
    json_object_set(root, buf.c_str(),json_real((float) delayTimeCells->cells[i]));
    buf = "feedback-" + std::to_string(i) ;
    json_object_set(root, buf.c_str(),json_real((float) feedbackCells->cells[i]));
    buf = "panning-" + std::to_string(i) ;
    json_object_set(root, buf.c_str(),json_real((float) panningCells->cells[i]));
  }

  return root;
}

void DelayedReactionModule::onReset() {

  delayRange = 2;

  // window function
  windowFunctionId = 4;

  attenuationLinked = false;
  delayTimeLinked = false;
  feedbackLinked = false;

  pinAttenuation0s = false;
  pinDelayTime0s = false;
  pinFeedback0s = false;
  pinPanning0s = false;

  // reset the cells
  attenuationCells->reset();
  delayTimeCells->reset();
  feedbackCells->reset();
  panningCells->reset();
}


float DelayedReactionModule::paramValue (uint16_t param, uint16_t input, float low, float high) {
  float current = params[param].getValue();

  if (inputs[input].isConnected()) {
    // high - low, divided by one tenth input voltage, plus the current value
    current += ((inputs[input].getVoltage() / 10) * (high - low));
  }

  return clamp(current, low, high);
}

void DelayedReactionModule::process(const ProcessArgs &args) {
  // get the current input
  float input = inputs[INPUT].getVoltage();
  // set the real value of the dry input
  dry->set(input);

  // get the dry input from the buffer, it should end up one loop delayed
  float dryDelayed = dry->get(); // use a real dry value, not something that we have altered


  // here check to see if the frameSize or the sample rate have changed
  if (args.sampleRate != sampleRate) {
    sampleRate = args.sampleRate;
  }

  bool masterPresent = (leftExpander.module && leftExpander.module->model == modelDelayedReaction);
  if (masterPresent) {
  // From Master
    float *messagesFromMaster = (float*)leftExpander.consumerMessage;

    uint8_t parameter = (uint8_t)messagesFromMaster[0];
    uint16_t position = (uint16_t)messagesFromMaster[1];
    uint16_t value = (uint16_t)messagesFromMaster[2];
    switch(parameter) {
      case 0:
        break; // Do nothing
      case 1:
        if(attenuationLinked)
          attenuationCells->setCell(value,position,false);
        break; // Attenuverter
      case 2:
        if(delayTimeLinked)
          delayTimeCells->setCell(value,position,false);
        break; // DelayTime
      case 3:
        if(feedbackLinked)
          feedbackCells->setCell(value,position,false);
        break; // FeedBack
    }
    leftExpander.messageFlipRequested = true;		
  }            
  

  if (lastFrameSize != frameSize) {
    lastFrameSize = frameSize;
    nbrBands = pow(2.0,frameSize-1);
    hopSize = frameSize - 2;

    delayAdjustment = pow(2,frameSize-11); //keep delay time constant, independent of frame sizze

    delete fft;
    fft = new FFT(nbrBands * 2);
    delete feedbackfft;
    feedbackfft = new FFT(nbrBands * 2);

    delete dry;
    dry = new Buffer<float>(pow(2.0f, frameSize), 0);
  
    delete windowFunction;
    windowFunction = new WindowFunction<float>(pow(2.0f, frameSize));
  
    for (uint8_t i = 0; i < MAX_FRAMES; i++) {
      delete dryBuffer[i];
      dryBuffer[i] = new Buffer<float>(pow(2.0f, frameSize),i * pow(2.0f, hopSize));

      delete feedbackResult[i];      
      feedbackResult[i] = new float[nbrBands*2] { 0 };

      delete processed[i];      
      processed[i] = new float[nbrBands*2] { 0 };

      curPos[i] = i * pow(2.0f,hopSize);
    }

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

  float pinXAxisPosAttenuation = paramValue(X_AXIS_PIN_POS_ATTENUATION, X_AXIS_PIN_POS_ATTENUATION_CV, 0, 1);
  attenuationXAxisPercentage = pinXAxisPosAttenuation;
  float xAxisRotationAttenuation = paramValue(X_AXIS_ROTATION_ATTENUATION, X_AXIS_ROTATION_ATTENUATION_CV, -1, 1);
  attenuationXAxisRotatePercentage = xAxisRotationAttenuation;
  if (pinAttenuation0sTrigger.process(params[PIN_ATTENUATION_0S].getValue())) {
    pinAttenuation0s = (pinAttenuation0s + 1) % 5;
  }
  attenuationCells->pinXAxisValues = pinAttenuation0s;
  attenuationCells->pinXAxisPosition = pinXAxisPosAttenuation;
  attenuationCells->rotateX = xAxisRotationAttenuation;
  switch (pinAttenuation0s) {
    case 0 :
      lights[PIN_ATTENUATION_0S_LIGHT+0].value = 0;
      lights[PIN_ATTENUATION_0S_LIGHT+1].value = 0;
      lights[PIN_ATTENUATION_0S_LIGHT+2].value = 0;
      break;
    case 1 :
      lights[PIN_ATTENUATION_0S_LIGHT].value = .15;
      lights[PIN_ATTENUATION_0S_LIGHT+1].value = 1;
      lights[PIN_ATTENUATION_0S_LIGHT+2].value = .15;
      break;
    case 2 :
      lights[PIN_ATTENUATION_0S_LIGHT].value = 0;
      lights[PIN_ATTENUATION_0S_LIGHT+1].value = 1;
      lights[PIN_ATTENUATION_0S_LIGHT+2].value = 0;
      break;
    case 3 :
      lights[PIN_ATTENUATION_0S_LIGHT].value = 1;
      lights[PIN_ATTENUATION_0S_LIGHT+1].value = .15;
      lights[PIN_ATTENUATION_0S_LIGHT+2].value = .15;
      break;
    case 4 :
      lights[PIN_ATTENUATION_0S_LIGHT].value = 1;
      lights[PIN_ATTENUATION_0S_LIGHT+1].value = 0;
      lights[PIN_ATTENUATION_0S_LIGHT+2].value = 0;
      break;
  }

  float pinXAxisPosDelayTime = paramValue(X_AXIS_PIN_POS_DELAY_TIME, X_AXIS_PIN_POS_DELAY_TIME_CV, 0, 1);
  delayTimeXAxisPercentage = pinXAxisPosDelayTime;
  float xAxisRotationDelayTime = paramValue(X_AXIS_ROTATION_DELAY_TIME, X_AXIS_ROTATION_DELAY_CV, -1, 1);
  delayTimeXAxisRotatePercentage = xAxisRotationDelayTime;
  if (pinDelayTime0sTrigger.process(params[PIN_DELAY_TIME_0S].getValue())) {
    pinDelayTime0s = (pinDelayTime0s + 1) % 5;
  }
  delayTimeCells->pinXAxisValues = pinDelayTime0s;
  delayTimeCells->pinXAxisPosition = pinXAxisPosDelayTime;
  delayTimeCells->rotateX = xAxisRotationDelayTime;
  switch (pinDelayTime0s) {
    case 0 :
      lights[PIN_DELAY_TIME_0S_LIGHT+0].value = 0;
      lights[PIN_DELAY_TIME_0S_LIGHT+1].value = 0;
      lights[PIN_DELAY_TIME_0S_LIGHT+2].value = 0;
      break;
    case 1 :
      lights[PIN_DELAY_TIME_0S_LIGHT].value = .15;
      lights[PIN_DELAY_TIME_0S_LIGHT+1].value = 1;
      lights[PIN_DELAY_TIME_0S_LIGHT+2].value = .15;
      break;
    case 2 :
      lights[PIN_DELAY_TIME_0S_LIGHT].value = 0;
      lights[PIN_DELAY_TIME_0S_LIGHT+1].value = 1;
      lights[PIN_DELAY_TIME_0S_LIGHT+2].value = 0;
      break;
    case 3 :
      lights[PIN_DELAY_TIME_0S_LIGHT].value = 1;
      lights[PIN_DELAY_TIME_0S_LIGHT+1].value = .15;
      lights[PIN_DELAY_TIME_0S_LIGHT+2].value = .15;
      break;
    case 4 :
      lights[PIN_DELAY_TIME_0S_LIGHT].value = 1;
      lights[PIN_DELAY_TIME_0S_LIGHT+1].value = 0;
      lights[PIN_DELAY_TIME_0S_LIGHT+2].value = 0;
      break;
  }

  float pinXAxisPosFeedback = paramValue(X_AXIS_PIN_POS_FEEDBACK, X_AXIS_PIN_POS_FEEDBACK_CV, 0, 1);
  feedbackXAxisPercentage = pinXAxisPosFeedback;
  float xAxisRotationFeedback = paramValue(X_AXIS_ROTATION_FEEDBACK, X_AXIS_ROTATION_FEEDBACK_CV, -1, 1);
  feedbackXAxisRotatePercentage = xAxisRotationFeedback;
  if (pinFeedback0sTrigger.process(params[PIN_FEEDBACK_0S].getValue())) {
    pinFeedback0s = (pinFeedback0s + 1) % 5;
  }
  feedbackCells->pinXAxisValues = pinFeedback0s;
  feedbackCells->pinXAxisPosition = pinXAxisPosFeedback;
  feedbackCells->rotateX = xAxisRotationFeedback;
  switch (pinFeedback0s) {
    case 0 :
      lights[PIN_FEEDBACK_0S_LIGHT+0].value = 0;
      lights[PIN_FEEDBACK_0S_LIGHT+1].value = 0;
      lights[PIN_FEEDBACK_0S_LIGHT+2].value = 0;
      break;
    case 1 :
      lights[PIN_FEEDBACK_0S_LIGHT].value = .15;
      lights[PIN_FEEDBACK_0S_LIGHT+1].value = 1;
      lights[PIN_FEEDBACK_0S_LIGHT+2].value = .15;
      break;
    case 2 :
      lights[PIN_FEEDBACK_0S_LIGHT].value = 0;
      lights[PIN_FEEDBACK_0S_LIGHT+1].value = 1;
      lights[PIN_FEEDBACK_0S_LIGHT+2].value = 0;
      break;
    case 3 :
      lights[PIN_FEEDBACK_0S_LIGHT].value = 1;
      lights[PIN_FEEDBACK_0S_LIGHT+1].value = .15;
      lights[PIN_FEEDBACK_0S_LIGHT+2].value = .15;
      break;
    case 4 :
      lights[PIN_FEEDBACK_0S_LIGHT].value = 1;
      lights[PIN_FEEDBACK_0S_LIGHT+1].value = 0;
      lights[PIN_FEEDBACK_0S_LIGHT+2].value = 0;
      break;
  }

  float pinXAxisPosPanning = paramValue(X_AXIS_PIN_POS_PANNING, X_AXIS_PIN_POS_PANNING_CV, 0, 1);
  panningXAxisPercentage = pinXAxisPosPanning;
  float xAxisRotationPanning = paramValue(X_AXIS_ROTATION_PANNING, X_AXIS_ROTATION_PANNING_CV, -1, 1);
  panningXAxisRotatePercentage = xAxisRotationPanning;
  if (pinPanning0sTrigger.process(params[PIN_PANNING_0S].getValue())) {
    pinPanning0s = (pinPanning0s + 1) % 5;
  }
  panningCells->pinXAxisValues = pinPanning0s;
  panningCells->pinXAxisPosition = pinXAxisPosPanning;
  panningCells->rotateX = xAxisRotationPanning;
  switch (pinPanning0s) {
    case 0 :
      lights[PIN_PANNING_0S_LIGHT+0].value = 0;
      lights[PIN_PANNING_0S_LIGHT+1].value = 0;
      lights[PIN_PANNING_0S_LIGHT+2].value = 0;
      break;
    case 1 :
      lights[PIN_PANNING_0S_LIGHT].value = .15;
      lights[PIN_PANNING_0S_LIGHT+1].value = 1;
      lights[PIN_PANNING_0S_LIGHT+2].value = .15;
      break;
    case 2 :
      lights[PIN_PANNING_0S_LIGHT].value = 0;
      lights[PIN_PANNING_0S_LIGHT+1].value = 1;
      lights[PIN_PANNING_0S_LIGHT+2].value = 0;
      break;
    case 3 :
      lights[PIN_PANNING_0S_LIGHT].value = 1;
      lights[PIN_PANNING_0S_LIGHT+1].value = .15;
      lights[PIN_PANNING_0S_LIGHT+2].value = .15;
      break;
    case 4 :
      lights[PIN_PANNING_0S_LIGHT].value = 1;
      lights[PIN_PANNING_0S_LIGHT+1].value = 0;
      lights[PIN_PANNING_0S_LIGHT+2].value = 0;
      break;
  }


  if (linkAttenuationTrigger.process(params[LINK_ATTENUATION].getValue())) {
    attenuationLinked = !attenuationLinked;
  }
  lights[LINK_ATTENUATION_LIGHT+0].value = attenuationLinked;
  lights[LINK_ATTENUATION_LIGHT+1].value = attenuationLinked;
  lights[LINK_ATTENUATION_LIGHT+2].value = attenuationLinked ? 0.2 : 0.0;

  if (linkDelayTimeTrigger.process(params[LINK_DELAY_TIME].getValue())) {
    delayTimeLinked = !delayTimeLinked;
  }
  lights[LINK_DELAY_TIME_LIGHT+0].value = delayTimeLinked;
  lights[LINK_DELAY_TIME_LIGHT+1].value = delayTimeLinked;
  lights[LINK_DELAY_TIME_LIGHT+2].value = delayTimeLinked ? 0.2 : 0.0;

  if (linkFeedbackTrigger.process(params[LINK_FEEDBACK].getValue())) {
    feedbackLinked = !feedbackLinked;
  }
  lights[LINK_FEEDBACK_LIGHT+0].value = feedbackLinked;
  lights[LINK_FEEDBACK_LIGHT+1].value = feedbackLinked;
  lights[LINK_FEEDBACK_LIGHT+2].value = feedbackLinked ? 0.2 : 0.0;


  

  if (delayRangeTrigger.process(params[DELAY_RANGE].getValue())) {
    delayRange = (delayRange + 1) % 6;
  }
  delayRangeTime =  ceil(baseDelaySizeinBytes * (1 << delayRange) / sampleRate) / 10;    
  switch (delayRange) {
    case 0 :
      lights[DELAY_RANGE_LIGHT+0].value = 0;
      lights[DELAY_RANGE_LIGHT+1].value = 0;
      lights[DELAY_RANGE_LIGHT+2].value = .5;
      break;
    case 1 :
      lights[DELAY_RANGE_LIGHT].value = .15;
      lights[DELAY_RANGE_LIGHT+1].value = .15;
      lights[DELAY_RANGE_LIGHT+2].value = .8;
      break;
    case 2 :
      lights[DELAY_RANGE_LIGHT].value = 0;
      lights[DELAY_RANGE_LIGHT+1].value = 1;
      lights[DELAY_RANGE_LIGHT+2].value = 0;
      break;
    case 3 :
      lights[DELAY_RANGE_LIGHT].value = 1;
      lights[DELAY_RANGE_LIGHT+1].value = 1;
      lights[DELAY_RANGE_LIGHT+2].value = 0;
      break;
    case 4 :
      lights[DELAY_RANGE_LIGHT].value = 1;
      lights[DELAY_RANGE_LIGHT+1].value = 0.5;
      lights[DELAY_RANGE_LIGHT+2].value = 0;
      break;
    case 5 :
      lights[DELAY_RANGE_LIGHT].value = 1;
      lights[DELAY_RANGE_LIGHT+1].value = 0;
      lights[DELAY_RANGE_LIGHT+2].value = 0;
      break;
  }


  //Expander Link logic
  bool slavePresent = (rightExpander.module && rightExpander.module->model == modelDelayedReaction);
  if(slavePresent) {
    uint8_t parameterClicked = 0; // Nothing clicked
    uint16_t lastPosition = 0;
    uint16_t lastValue = 0;
    
    messageToSlave = (float*)(rightExpander.module->leftExpander.producerMessage);	
    if(attenuationCells->readyForExpander) { 
      parameterClicked = 1;
      lastPosition = attenuationCells->lastPosition;
      lastValue = attenuationCells->lastValue;
      attenuationCells->readyForExpander = false;
    } else if(delayTimeCells->readyForExpander) { 
      parameterClicked = 2;
      lastPosition = delayTimeCells->lastPosition;
      lastValue = delayTimeCells->lastValue;
      delayTimeCells->readyForExpander = false;
    } else if(feedbackCells->readyForExpander) { 
      parameterClicked = 3;
      lastPosition = feedbackCells->lastPosition;
      lastValue = feedbackCells->lastValue;
      feedbackCells->readyForExpander = false;
    }
     
    messageToSlave[0] = parameterClicked;
    messageToSlave[1] = lastPosition;
    messageToSlave[2] = lastValue;
  }  

 
  float attenuationShiftX = inputs[ATTENUATION_X_CV].getVoltage() / 5.0;
  float attenuationShiftY = inputs[ATTENUATION_Y_CV].getVoltage() / 5.0;
  attenuationCells->shiftX = attenuationShiftX;
  attenuationCells->shiftY = attenuationShiftY;

  float delayTimeShiftX = inputs[DELAY_TIME_X_CV].getVoltage() / 5.0;
  float delayTimeShiftY = inputs[DELAY_TIME_Y_CV].getVoltage() / 5.0;
  delayTimeCells->shiftX = delayTimeShiftX;
  delayTimeCells->shiftY = delayTimeShiftY;
  
  float feedbackShiftX = inputs[DELAY_FEEDBACK_X_CV].getVoltage() / 5.0;
  float feedbackShiftY = inputs[DELAY_FEEDBACK_Y_CV].getVoltage() / 5.0;
  feedbackCells->shiftX = feedbackShiftX;
  feedbackCells->shiftY = feedbackShiftY;

  float panningShiftX = inputs[PANNING_X_CV].getVoltage() / 5.0;
  float panningShiftY = inputs[PANNING_Y_CV].getVoltage() / 5.0;
  panningCells->shiftX = panningShiftX;
  panningCells->shiftY = panningShiftY;

  for(uint8_t i=0; i < MAX_FRAMES; i++) {
    float windowedValue = windowFunction->windowValue(windowFunctionId,dryBuffer[i]->setPos);
    dryBuffer[i]->set(input * windowedValue + feedback); 
    
    // is it time to do the fft?
    if (dryBuffer[i]->looped) {
      curPos[i] = 0;
      fft->fft(dryBuffer[i]->data);

      uint16_t fftBand = nbrBands; 
      for(uint16_t uiBand=0;uiBand < NUM_UI_BANDS;uiBand++) {

        float attenuation = attenuationCells->valueForPosition(uiBand);
        float delayTime = delayTimeCells->displayValueForPosition(uiBand);
        float feedbackAmount = feedbackCells->valueForPosition(uiBand);
        float panningValue = panningCells->valueForPosition(uiBand);

        float bandSum = 0;
        for (uint16_t j = 0; j < bandsPerUIBand[uiBand]; j++) { 
          fftBand-=1;
          delayLine[fftBand].setDelayTime(delayTime * ((1 << delayRange) * 0.64 / delayAdjustment));
          bandPanning[fftBand] = panningValue;

          kiss_fft_cpx inputValue = fft->out[fftBand]; 
          float magnitude = sqrt(inputValue.r * inputValue.r + inputValue.i * inputValue.i);
          float phase = atan2(inputValue.i, inputValue.r);

          bandSum += magnitude;
          magnitude = magnitude * attenuation;

          inputValue.r = magnitude*cos(phase);
          inputValue.i = magnitude*sin(phase);

          delayLine[fftBand].write(inputValue);

          
          //Process Feedback
          kiss_fft_cpx delayedValue = delayLine[fftBand].getValue(); 

          fft->in[fftBand].r = delayedValue.r;
          fft->in[fftBand].i = delayedValue.i;

          magnitude = sqrt(delayedValue.r * delayedValue.r + delayedValue.i * delayedValue.i);
          phase = atan2(delayedValue.i, delayedValue.r);

          if(fftBand == 0)
            feedbackAmount = 0;

          magnitude = magnitude * feedbackAmount;
          
          
          float newR = magnitude*cos(phase);
          float newI = magnitude*sin(phase);

          feedbackfft->in[fftBand].r = newR;
          feedbackfft->in[fftBand].i = newI;
        }
        spectrograph[uiBand] = bandSum / (float(bandsPerUIBand[uiBand]) * nbrBands * 4);
        
      }

      for(uint16_t padIndex = nbrBands; padIndex < nbrBands*2;padIndex++) {
          fft->in[padIndex].r = 0;
          fft->in[padIndex].i = 0;
          feedbackfft->in[padIndex].r = 0;
          feedbackfft->in[padIndex].i = 0;
      }

      fft->ifft(processed[i]);      
      feedbackfft->ifft(feedbackResult[i]);      
    }
  }


  float mix = paramValue(MIX, MIX_CV, 0, 1);
  // set the mix percentage
  mixPercentage = float(mix * 100.0) / 100.0;

  float proceesedTotal = 0;
  float proceesedTotalL = 0;
  float proceesedTotalR = 0;
  float feedbackTotal = 0;

  for(int i=0;i<MAX_FRAMES;i++) {
    float processedValue = processed[i][curPos[i]] * windowFunction->windowValue(windowFunctionId,curPos[i]);
    float panningValue = bandPanning[i];

    proceesedTotal += processedValue;
    proceesedTotalL += processedValue * (panningValue > 0 ? (1.0 - panningValue) : 1.0);
    proceesedTotalR += processedValue * (panningValue < 0 ? (1.0 - abs(panningValue)) : 1.0);

    feedbackTotal += feedbackResult[i][curPos[i]] * windowFunction->windowValue(windowFunctionId,curPos[i]);
    curPos[i]++;
  }

  feedbackTotal = clamp(feedbackTotal,-10.0,10.0);
  
  outputs[FEEDBACK_SEND].setVoltage(feedbackTotal);
  if(inputs[FEEDBACK_RETURN].isConnected()) {
    feedback = inputs[FEEDBACK_RETURN].getVoltage();
  } else {
    feedback = feedbackTotal;
  }
  

  if(!outputs[OUTPUT_R].isConnected()) { //Mono Mode
    float wet = proceesedTotal;
    float output = (wet * mix) + ((1.0f - mix) * dryDelayed);

    outputs[OUTPUT_L].setVoltage(output);
  } else {
    float wetL = proceesedTotalL;
    float wetR = proceesedTotalR;
    float outputL = (wetL * mix) + ((1.0f - mix) * dryDelayed);
    float outputR = (wetR * mix) + ((1.0f - mix) * dryDelayed);

    outputs[OUTPUT_L].setVoltage(outputL);
    outputs[OUTPUT_R].setVoltage(outputR);
  }
}
