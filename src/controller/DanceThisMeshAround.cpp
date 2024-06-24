#include "DanceThisMeshAround.hpp"
#include <cmath>
//#define DR_WAV_IMPLEMENTATION
#include "../model/dr_wav.h"
#include "../model/Interpolate.hpp"

//using namespace frequencydomain::dsp;

DanceThisMeshAroundModule::DanceThisMeshAroundModule() {
  config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

  sampleRate = APP->engine->getSampleRate();


  configParam(MESH_TOPOLOGY_PARAM, 0.0f, 2.0f, 0.0f, "Mesh Configuration");
  configParam(MESH_SIZE_PARAM, 2.0f, 5.0f, 2.0f, "Mesh Size");


  configParam(INPUT_NONLINEARITY_PARAM, 1.f, 8.f, 1.0f, "Input Non-linearity", "x");

  configParam(BP_1_CUTOFF_PARAM, 0.29f, 0.5f, 0.35f, "WG 1 Filter Fc", " Hz", std::pow(2, 10.f), dsp::FREQ_C4 / std::pow(2, 5.f));
  configParam(BP_2_CUTOFF_PARAM, 0.31f, 0.75f, 0.5f, "WG 2 Filter Fc", " Hz", std::pow(2, 10.f), dsp::FREQ_C4 / std::pow(2, 5.f));
  configParam(BP_3_CUTOFF_PARAM, 0.34f, 1.f, 0.75f, "Mesh Filter Fc", " Hz", std::pow(2, 10.f), dsp::FREQ_C4 / std::pow(2, 5.f));

  configParam(GROUP_FEEDBACK_PARAM, 0.f, 1.f, 0.5f, "Feedback Amount", " %",0,100);
  configParam(INPUT_NONLINEARITY_PARAM, 1.f, 5.f, 1.0f, "Feedback Non-linearity", "x");

  configParam(DELAY_TIME_1_PARAM, 1.f, 500.f, 50.0f, "WG 1 Delay Time", " Samples");
  configParam(DELAY_TIME_2_PARAM, 1.f, 500.f, 50.0f, "WG 2 Delay Time", " Samples");

  configParam(DELAY_TIME_MESH_PARAM, 1.f, 500.f, 50.0f, "Mesh Delay Time", " Samples");
  configParam(DELAY_MESH_FB_AMOUNT_PARAM, 0.f, 1.f, 0.5f, "Mesh X Axis Feedback Amount", " %",0,100);
  configParam(DELAY_MESH_FB_NONLINEARITY_PARAM, 1.f, 5.f, 1.0f, "Mesh X Axis Feedback Non-linearity", "x");


  configParam(MESH_IMPEDANCE_PARAM, 0.0f, 1.0f, 1.0f, "Mesh Impedance", "%", 0,100);


  configInput(IMPULSE_INPUT, "Impulse");
  configInput(MESH_SIZE_INPUT, "Mesh Size CV");
  configInput(INPUT_NONLINEARITY_INPUT, "Non-linearity CV");

  configInput(BP_1_CUTOFF_INPUT, "WG 1 Filter Fc CV");
  configInput(BP_2_CUTOFF_INPUT, "WG 2 Filter Fc CV");
  configInput(BP_3_CUTOFF_INPUT, "Mesh Filter Fc CV");

  configInput(DELAY_TIME_1_INPUT, "WG 1 Delay Time CV");
  configInput(DELAY_TIME_2_INPUT, "WG 2 Delay Time CV");
  configInput(DELAY_TIME_MESH_INPUT, "Mesh X Axis Delay Time CV");

  configInput(DELAY_1_FB_AMOUNT_INPUT, "WG 1 Delay Feedback Amount CV");
  configInput(DELAY_2_FB_AMOUNT_INPUT, "WG 2 Delay Feedback Amount CV");
  configInput(DELAY_MESH_FB_AMOUNT_INPUT, "Mesh X Axis Feedback Amount CV");
  configInput(DELAY_MESH_FB_NONLINEARITY_INPUT, "Mesh X Axis Feedback Non-linearity CV");

  configInput(MESH_IMPEDANCE_INPUT, "Mesh Impedance CV");

  configInput(GROUP_FEEDBACK_INPUT, "Feedback Ammount CV");

  configInput(ALLPASS_FC_INPUT, "Allpass Fc CV");

  configOutput(OUTPUT_1, "Mesh");



  junctions.resize(0);
  delayLines.resize(0);
  mesh.resize(0);
  
  harmonicShiftCells = new OneDimensionalCellsWithRollover(32, 16, -4 , 4, PIN_ROLLOVER_MODE,WRAP_AROUND_ROLLOVER_MODE);

  for(int i=0;i<MAX_BANDS;i++) {
    bandpassFilters[i*2] = new NonlinearBiquad<double>(bq_type_bandpass,0.0125*(i*2+1),0.5,0.0);
    bandpassFilters[i*2]->setNonLinearType(NLBQ_NONE);
    bandpassFilters[i*2+1] = new NonlinearBiquad<double>(bq_type_bandpass,0.0125*(i*2+1),0.5,0.0);
    bandpassFilters[i*2+1]->setNonLinearType(NLBQ_NONE);

    // allpassFilters[i] = new Biquad<double>(bq_type_allpass,0.0125*(i*2+1),0.5,0.0);
  }

  srand(time(NULL));

  onReset();
}

DanceThisMeshAroundModule::~DanceThisMeshAroundModule() {
  delete harmonicShiftCells;
}

void DanceThisMeshAroundModule::onReset() {

  junctions.resize(0);
  delayLines.resize(0);
  mesh.resize(0);
}

void DanceThisMeshAroundModule::dataFromJson(json_t *root) {

}

json_t *DanceThisMeshAroundModule::dataToJson() {

  json_t *root = json_object();
 
  return root;
}

float DanceThisMeshAroundModule::paramValue (uint16_t param, uint16_t input, float low, float high) {
  float current = params[param].getValue();

  if (inputs[input].isConnected()) {
    // high - low, divided by one tenth input voltage, plus the current value
    current += ((inputs[input].getVoltage() / 10) * (high - low));
  }

  return clamp(current, low, high);
}



void DanceThisMeshAroundModule::process(const ProcessArgs &args) {
    meshTopology = params[MESH_TOPOLOGY_PARAM].getValue();
    topologyPercentage = meshTopology / (NBR_MESH_TOPOLOGIES - 1.0);

    meshSize = paramValue(MESH_SIZE_PARAM,MESH_SIZE_INPUT,2.0,5.0);
    meshSizePercentage = (meshSize-2.0) / 3.0;

    if(meshTopology != lastMeshTopology || meshSize != lastMeshSize) {
      
      int zSize = 1;
      if (meshTopology == MESH_CUBIC || meshTopology == MESH_TETRAHEDRAL) {
        zSize = meshSize;
      }
      
      junctions.resize(zSize);
      delayLines.resize(zSize+1);

      for(int k = 0;k<=zSize;k++) {

        if(k<zSize) {
          junctions[k].resize(meshSize);
          for(int i=0;i<meshSize;i++) {
            junctions[k][i].resize(meshSize);
          }
        }


        delayLines[k].resize(meshSize+1);
        for(int i=0;i<=meshSize;i++) {
          delayLines[k][i].resize(meshSize+1);
          for(int j=0;j<=meshSize;j++) {
            delayLines[k][i][j].resize(meshTopology == MESH_RECTILINEAR ? 4 : 6);
          }
        }
      }
      lastMeshTopology = meshTopology;
      lastMeshSize =  meshSize;

      resolvedIXPostion = std::floor(meshSize / 2.0);
      resolvedIYPostion = std::floor(meshSize / 2.0);
      resolvedIZPostion = std::floor(zSize / 2.0);

      resolvedO1XPostion = std::round(meshSize-1);
      resolvedO1YPostion = std::round(meshSize-1);
      resolvedO1ZPostion = std::floor(zSize-1);


    }


    for(int b=0;b<MAX_BANDS;b++) {
    // Get pitch
      bpCutoff[b] = paramValue(BP_1_CUTOFF_PARAM+b,BP_1_CUTOFF_INPUT+b,0.05,1.0);
      if(b > 0 && bpCutoff[b] / bpCutoff[b-1] < 1.20 ) { // Prevent Bands from crossing
        bpCutoff[b] = bpCutoff[b-1] * 1.20;
      } 

      if(bpCutoff[b] != lastBpCutoff[b]) {
        float cutoff = dsp::FREQ_C4 * pow(2.f, bpCutoff[b] * 10.f - 5.f);
        cutoff = clamp(cutoff, 60.f, 10000.f) / 22000;        
        bandpassFilters[b*2]->setFilterParameters(bq_type_bandpass,cutoff,1.707,nonlinearity[b],0.0);
        bandpassFilters[b*2+1]->setFilterParameters(bq_type_bandpass,cutoff,1.707,nonlinearity[b],0.0);
        lastBpCutoff[b] = bpCutoff[b];

        // fprintf(stderr, "Band Pass Filter %i  Fc:%f %f \n",b,bpCutoff[b]);
      }
    }
    bpCutoff1Percentage = bpCutoff[0];
    bpCutoff2Percentage = bpCutoff[1];
    bpCutoffMeshPercentage = bpCutoff[2];


    // apCutoff = paramValue(ALLPASS_FC_PARAM,ALLPASS_FC_INPUT,0.05,0.5);
  
    // if(apCutoff != lastApCutoff) {
    //   float cutoff = dsp::FREQ_C4 * pow(2.f, apCutoff * 10.f - 5.f);
    //   cutoff = clamp(cutoff, 60.f, 15000.f) / 22000;        
    //   for(int b=0;b<MAX_BANDS;b++) {
    //     allpassFilters[b]->setFilterParameters(bq_type_allpass,cutoff,0.707,1.0,0.0);
    //   }
    //   lastApCutoff = apCutoff;
    // }
  

    // Get Input

    double inputNonlinearity = paramValue(INPUT_NONLINEARITY_PARAM,INPUT_NONLINEARITY_INPUT,1.0,5.0);
    inputNonlinerityPercentage = (inputNonlinearity - 1.0) / 4.0;

    double feedback = paramValue(GROUP_FEEDBACK_PARAM,GROUP_FEEDBACK_INPUT,0.0,1.0);; // this will be parameterized - move this to top
    groupFeedbackPercentage = feedback;

    //double input = hardClip(inputs[IMPULSE_INPUT].getVoltage()/5.0 + lastOutput,inputNonlinearity); //scale
    double input = inputs[IMPULSE_INPUT].getVoltage()/5.0 + (lastOutput * feedback); 
    input = std::max(std::min(input * inputNonlinearity, 1.0), -1.0); //hard limit




    // Process WaveGuides
    float waveguidesOut = 0.0;
    int delayTime[2];
    for(int b=0;b<MAX_BANDS-1;b++) {
      float filterOut = bandpassFilters[b*2+1]->process(bandpassFilters[b*2]->process(input));
      waveguideDelays[b].write(filterOut);
      delayTime[b] = paramValue(DELAY_TIME_1_PARAM+b,DELAY_TIME_1_INPUT+b,1.0,500.0);
      float waveguideOut = waveguideDelays[b].getNonInterpolatedDelay(delayTime[b]);
        //fprintf(stderr, "Band Pass Filter %i delayTime:%i output:%f \n",b,delayTime,waveguideOut);
      waveguidesOut +=waveguideOut;
    }
    delayTime1Percentage = delayTime[0] / 500.0;
    delayTime2Percentage = delayTime[1] / 500.0;


    //Delay order - N S W E
    //Process Mesh - just Rectilinear to start
    int delayTimeMesh = paramValue(DELAY_TIME_MESH_PARAM,DELAY_TIME_MESH_INPUT,1.0,500.0) * 2.0;
    float meshFBAmount = paramValue(DELAY_MESH_FB_AMOUNT_PARAM,DELAY_MESH_FB_AMOUNT_INPUT,0.0,0.95);
    double meshFBNonlinearity = paramValue(DELAY_MESH_FB_NONLINEARITY_PARAM,DELAY_MESH_FB_NONLINEARITY_INPUT,1.0,5.0);
    // float impedance = paramValue(MESH_IMPEDANCE_PARAM,MESH_IMPEDANCE_INPUT,0.0,1.0);


    delayTimeMeshPercentage = delayTimeMesh / 1000.0;
    delayTimeMeshFeedbackPercentage = meshFBAmount;
    delayTimeMeshNonLinearityPercentage = (meshFBNonlinearity- 1.0) / 4.0;


    float meshfilterOut = bandpassFilters[(MAX_BANDS*2)-1]->process(bandpassFilters[(MAX_BANDS-1)*2]->process(input));
    junctions[resolvedIZPostion][resolvedIYPostion][resolvedIXPostion].AddExternalInput(meshfilterOut);

    //fprintf(stderr, "rx:%i ry:%i mo:%f \n",resolvedIXPostion,resolvedIYPostion,meshfilterOut);

    float impedance = 1.0;
    if(meshTopology == MESH_RECTILINEAR) {
      //Process Inputs
      for(int y=0;y<meshSize;y++) {
        for(int x=0;x<meshSize;x++) {
            float in1 = delayLines[0][y][x][0].getNonInterpolatedDelay(delayTimeMesh); //w
            float in2 = delayLines[0][y][x][1].getNonInterpolatedDelay(delayTimeMesh); //n
            float in3 = delayLines[0][y][x+1][2].getNonInterpolatedDelay(delayTimeMesh); //e
            float in4 = delayLines[0][y+1][x][3].getNonInterpolatedDelay(delayTimeMesh); //s

            junctions[0][y][x].RectilinearJunction(in1,in2,in3,in4);        
        }
      }


      //Process Outputs
      for(int y=0;y<meshSize;y++) {
        for(int x=0;x<meshSize;x++) {

          MeshJunction<float> junction = junctions[0][y][x];

          delayLines[0][y][x+1][0].write((junction.currentValue - junction.in3) * impedance); //e
          delayLines[0][y+1][x][1].write((junction.currentValue - junction.in4) * impedance);// s
          delayLines[0][y][x][2].write((junction.currentValue - junction.in1) * impedance); //w
          delayLines[0][y][x][3].write((junction.currentValue - junction.in2) * impedance); //n

        }
      }

      //Process Edges
      for(int e=0;e<meshSize;e++) {
          float inW = delayLines[0][e][0][2].getNonInterpolatedDelay(delayTimeMesh);
          inW = std::max(std::min(meshFBNonlinearity * inW, 1.0), -1.0); //scale
          delayLines[0][e][0][0].write(-inW * meshFBAmount); // reflection

          float inN = delayLines[0][0][e][3].getNonInterpolatedDelay(delayTimeMesh);
          inN = std::max(std::min(meshFBNonlinearity * inN, 1.0), -1.0); //scale
          delayLines[0][0][e][1].write(-inN * meshFBAmount); // reflection

          float inE = delayLines[0][e][meshSize][0].getNonInterpolatedDelay(delayTimeMesh);
          inE = std::max(std::min(meshFBNonlinearity * inE, 1.0), -1.0); //scale
          delayLines[0][e][meshSize][2].write(-inE * meshFBAmount); // reflection

          float inS = delayLines[0][meshSize][e][1].getNonInterpolatedDelay(delayTimeMesh);
          inS = std::max(std::min(meshFBNonlinearity * inS, 1.0), -1.0); //scale
          delayLines[0][meshSize][e][3].write(-inS * meshFBAmount); // reflection

      }
    } else if(meshTopology == MESH_TRIANGLE) { // Triangular
// fprintf(stderr, "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!1  Doing triangular shit! \n");
      //Process Inputs
      for(int y=0;y<meshSize;y++) {
        for(int x=0;x<=y;x++) {
          float in1 = delayLines[0][y][x][0].getNonInterpolatedDelay(delayTimeMesh); //w
          float in2 = delayLines[0][y][x][1].getNonInterpolatedDelay(delayTimeMesh); //nw
          float in3 = delayLines[0][y][x][2].getNonInterpolatedDelay(delayTimeMesh); //ne
          float in4 = delayLines[0][y][x+1][3].getNonInterpolatedDelay(delayTimeMesh); //e
          float in5 = delayLines[0][y+1][x+1][4].getNonInterpolatedDelay(delayTimeMesh); //se
          float in6 = delayLines[0][y+1][x][5].getNonInterpolatedDelay(delayTimeMesh); //sw

          junctions[0][y][x].TriangularJunction(in1,in2,in3,in4,in5,in6);
        }
      }


      //Process Outputs
      for(int y=0;y<meshSize;y++) {
        for(int x=0;x<=y;x++) {

          MeshJunction<float> junction = junctions[0][y][x];

          delayLines[0][y][x+1][0].write(junction.currentValue - junction.in4 * impedance); //e
          delayLines[0][y+1][x+1][1].write(junction.currentValue - junction.in5 * impedance); //se
          delayLines[0][y+1][x][2].write(junction.currentValue - junction.in6 * impedance); //sw
          delayLines[0][y][x][3].write(junction.currentValue - junction.in1 * impedance); //w
          delayLines[0][y][x][4].write(junction.currentValue - junction.in2 * impedance); //nw
          delayLines[0][y][x][5].write(junction.currentValue - junction.in3 * impedance); //ne

        }
      }

      //Process Edges
      for(int e=0;e<meshSize;e++) {
          float inW = delayLines[0][e][0][3].getNonInterpolatedDelay(delayTimeMesh);
          inW = std::max(std::min(meshFBNonlinearity * inW, 1.0), -1.0); //scale
          delayLines[0][e][0][0].write(-inW * meshFBAmount); // reflection

          float inNW = delayLines[0][e][0][4].getNonInterpolatedDelay(delayTimeMesh);
          inNW = std::max(std::min(meshFBNonlinearity * inNW, 1.0), -1.0); //scale
          delayLines[0][e][0][1].write(-inNW * meshFBAmount); // reflection

          float inNE = delayLines[0][e][e][5].getNonInterpolatedDelay(delayTimeMesh);
          inNE = std::max(std::min(meshFBNonlinearity * inNE, 1.0), -1.0); //scale
          delayLines[0][e][e][2].write(-inNE * meshFBAmount); // reflection

          float inE = delayLines[0][e][e+1][0].getNonInterpolatedDelay(delayTimeMesh);
          inE = std::max(std::min(meshFBNonlinearity * inE, 1.0), -1.0); //scale
          delayLines[0][e][e+1][3].write(-inE * meshFBAmount); // reflection

          float inSE = delayLines[0][e+1][e+1][1].getNonInterpolatedDelay(delayTimeMesh);
          inSE = std::max(std::min(meshFBNonlinearity * inSE, 1.0), -1.0); //scale
          delayLines[0][e+1][e+1][4].write(-inSE * meshFBAmount); // reflection

          float inSW = delayLines[0][e+1][e][2].getNonInterpolatedDelay(delayTimeMesh);
          inSW = std::max(std::min(meshFBNonlinearity * inSW, 1.0), -1.0); //scale
          delayLines[0][e+1][e][5].write(-inSW * meshFBAmount); // reflection
      }
    } else if(meshTopology == MESH_CUBIC) {
      //Process Inputs
      for(int z=0;z<meshSize;z++) {
        for(int y=0;y<meshSize;y++) {
          for(int x=0;x<meshSize;x++) {
              float in1 = delayLines[z][y][x][0].getNonInterpolatedDelay(delayTimeMesh); //f
              float in2 = delayLines[z][y][x][1].getNonInterpolatedDelay(delayTimeMesh); //w
              float in3 = delayLines[z][y][x][2].getNonInterpolatedDelay(delayTimeMesh); //n
              float in4 = delayLines[z+1][y][x][3].getNonInterpolatedDelay(delayTimeMesh); //b
              float in5 = delayLines[z][y][x+1][4].getNonInterpolatedDelay(delayTimeMesh); //e
              float in6 = delayLines[z][y+1][x][5].getNonInterpolatedDelay(delayTimeMesh); //s

              junctions[z][y][x].CubicJunction(in1,in2,in3,in4,in5,in6);        
          }
        }
      }

      //Process Outputs
      for(int z=0;z<meshSize;z++) {
        for(int y=0;y<meshSize;y++) {
          for(int x=0;x<meshSize;x++) {

            MeshJunction<float> junction = junctions[z][y][x];

            delayLines[z+1][y][x][0].write(junction.currentValue - junction.in4 * impedance); //b
            delayLines[z][y][x+1][1].write(junction.currentValue - junction.in5 * impedance);// s
            delayLines[z][y+1][x][2].write(junction.currentValue - junction.in6 * impedance); //e
            delayLines[z][y][x][3].write(junction.currentValue - junction.in1 * impedance); //f
            delayLines[z][y][x][4].write(junction.currentValue - junction.in2 * impedance); //w
            delayLines[z][y][x][5].write(junction.currentValue - junction.in3 * impedance); //n

          }
        }
      }
      //Process Edges
      for(int e1=0;e1<meshSize;e1++) {
        for(int e2=0;e2<meshSize;e2++) { 
            float inF = delayLines[0][e1][e2][3].getNonInterpolatedDelay(delayTimeMesh);
            inF = std::max(std::min(meshFBNonlinearity * inF, 1.0), -1.0); //scale
            delayLines[0][e1][e2][0].write(-inF * meshFBAmount); // reflection

            float inW = delayLines[e1][e2][0][4].getNonInterpolatedDelay(delayTimeMesh);
            inW = std::max(std::min(meshFBNonlinearity * inW, 1.0), -1.0); //scale
            delayLines[e1][e2][0][1].write(-inW * meshFBAmount); // reflection

            float inN = delayLines[e1][0][e2][5].getNonInterpolatedDelay(delayTimeMesh);
            inN = std::max(std::min(meshFBNonlinearity * inN, 1.0), -1.0); //scale
            delayLines[e1][0][e2][2].write(-inN * meshFBAmount); // reflection

            float inB = delayLines[meshSize][e1][e2][0].getNonInterpolatedDelay(delayTimeMesh);
            inB = std::max(std::min(meshFBNonlinearity * inB, 1.0), -1.0); //scale
            delayLines[meshSize][e1][e2][3].write(-inB * meshFBAmount); // reflection

            float inE = delayLines[e1][e2][meshSize][1].getNonInterpolatedDelay(delayTimeMesh);
            inE = std::max(std::min(meshFBNonlinearity * inE, 1.0), -1.0); //scale
            delayLines[e1][e2][meshSize][4].write(-inE * meshFBAmount); // reflection

            float inS = delayLines[e1][meshSize][e2][2].getNonInterpolatedDelay(delayTimeMesh);
            inS = std::max(std::min(meshFBNonlinearity * inS, 1.0), -1.0); //scale
            delayLines[e1][meshSize][e2][5].write(-inS * meshFBAmount); // reflection
        }
      }
    }


    float meshOutput = junctions[resolvedO1ZPostion][resolvedO1YPostion][resolvedO1XPostion].currentValue;
    waveguidesOut += meshOutput;

    lastOutput = waveguidesOut;
    outputs[OUTPUT_1].setVoltage(waveguidesOut * 5.0);
  // }

      
};
