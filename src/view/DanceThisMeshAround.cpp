#include "../controller/DanceThisMeshAround.hpp"

#include "../component/knob.hpp"
#include "../component/light.hpp"
#include "../component/port.hpp"
#include "../component/button.hpp"
#include "../component/display.hpp"
#include "../component/menu.hpp"



struct DMDisplayFilterResponse : FramebufferWidget {
  DanceThisMeshAroundModule *module;

  DMDisplayFilterResponse() {
  }


  void draw(const DrawArgs &args) override {
    //background
    nvgFillColor(args.vg, nvgRGB(20, 30, 33));
    nvgBeginPath(args.vg);
    nvgRect(args.vg, 0, 0, box.size.x, box.size.y);
    nvgFill(args.vg);

    nvgBeginPath(args.vg);
    nvgStrokeColor(args.vg, nvgRGB(0x50,0x50,0x50));
    nvgStrokeWidth(args.vg, 0.5);
    for(int i=1;i<10;i+=1) {
        float xPosition = (std::log10(i) * 54.7);
        nvgMoveTo(args.vg,xPosition,0.5);
        nvgLineTo(args.vg,xPosition,100.5);
    }
    for(int i=10;i<100;i+=10) {
        float xPosition = (std::log10(i) * 54.7);
        nvgMoveTo(args.vg,xPosition,0.5);
        nvgLineTo(args.vg,xPosition,100.5);
    }
    float xPosition = (std::log10(150) * 54.7);
    nvgMoveTo(args.vg,xPosition,0.5);
    nvgLineTo(args.vg,xPosition,100.5);
    nvgStroke(args.vg);

    for(int i=1;i<6;i++) {
      nvgBeginPath(args.vg);
      //nvgStrokeColor(args.vg, i !=3 ? nvgRGB(0x50,0x50,0x50) : nvgRGB(0xa0,0xa0,0xa0));
      nvgStrokeColor(args.vg, nvgRGB(0x50,0x50,0x50));
      nvgMoveTo(args.vg,0.5,i*16);
      nvgLineTo(args.vg,128.5,i*16);
      nvgStroke(args.vg);
    }

    if (!module) 
      return;

        //fprintf(stderr, "Point x:%i l:%i freq:%f response:%f  gain: %f  \n",x,l,frequency,response,levelResponse);

    for(int s=0;s<MAX_BANDS;s++) {
      float driveLevel = module->nonlinearity[s]; 
      float r = interpolate(176.0f,255.0f,driveLevel,0.0f,5.0f);
      float g = interpolate(176.0f,0.0f,driveLevel,0.0f,5.0f);
      float b = interpolate(255.0f,0.0f,driveLevel,0.0f,5.0f);
      nvgBeginPath(args.vg);  
      //nvgStrokeColor(args.vg, nvgRGB(0xb0,0xb0,0xff));
      nvgStrokeColor(args.vg, nvgRGB(r,g,b));
      nvgStrokeWidth(args.vg, 1);
      for(float x=0.0f; x<128.0f; x+=1.0f) {
        double frequency = std::pow(10.0f, x/54.7f + 2.0f) / module->sampleRate;
        double response = module->bandpassFilters[s*2]->frequencyResponse(frequency); 
          // fprintf(stderr, "Point x:%i l:%i freq:%f response:%f  level response: %f  \n",x,l,frequency[0],response[0],levelResponse[0]);
        double responseDB = std::log10(std::max(response * response, 1.0e-4)) * -38;
        float responseYCoord = clamp((float) responseDB, 1.0f, 99.0f);

        if(x < 1)
          nvgMoveTo(args.vg, 0.0f, responseYCoord);
        else  
          nvgLineTo(args.vg, x, responseYCoord);
    
    }
    nvgStroke(args.vg);
  }
}

};    

struct DMDisplayMesh : FramebufferWidget {
  DanceThisMeshAroundModule *module;
  std::shared_ptr<Font> font;

  DMDisplayMesh() {
    font = APP->window->loadFont(asset::plugin(pluginInstance, "res/fonts/routed-gothic.ttf"));
  }

  inline float getYPos(int y,float spacing) {
    return y*spacing + 10.0;
  }

  inline float getCubicYPos(int y,float z,float spacing) {
    return (y - z*0.5) * spacing + 35.0;
  }

  inline float getRectXPos(int x,float spacing) {
    return x*spacing + 10.0;
  }

  inline float getTriYPos(int x,int y,float spacing) {
    return x*spacing + (module->meshSize -y - 1.0) * (spacing/2.0) + 10.0;
  }

  inline float getCubicXPos(int x,float z,float spacing) {
    return (x + z*0.5) * spacing + 10.0;
  }

  void draw(const DrawArgs &args) override {
    //background
    nvgFillColor(args.vg, nvgRGB(20, 30, 33));
    nvgBeginPath(args.vg);
    nvgRect(args.vg, 0, 0, box.size.x, box.size.y);
    nvgFill(args.vg);


    if (!module) 
      return;

    float z = 2;
    float spacing = 80.0 / (module->meshSize-1);
    float spacing3D = 50.0 / (module->meshSize-1);
    float meshSizef = float(module->meshSize) - 1.0;
    nvgFillColor(args.vg,nvgRGB(0x1f,0xd0,0x1f)); 	
    nvgStrokeWidth(args.vg, 0.5);
    nvgStrokeColor(args.vg, nvgRGB(0xb0,0xb0,0x2f));

    //Draw lines
    for(int i=0;i<module->meshSize;i++) {
      float yPos;
      if(module->meshTopology == module->MESH_RECTILINEAR) {
        yPos = getYPos(i,spacing);
        for(int j=0;j<module->meshSize;j++) {
          float xPos = getRectXPos(j,spacing);
          nvgBeginPath(args.vg);
          if(j > 0) {
            nvgMoveTo(args.vg,xPos,yPos);
            nvgLineTo(args.vg,getRectXPos(j-1,spacing),yPos);
          }
          if(i > 0) {
            nvgMoveTo(args.vg,xPos,yPos);
            nvgLineTo(args.vg,xPos,getYPos(i-1,spacing));
          }
          nvgStroke(args.vg);
        }
      } else if(module->meshTopology == module->MESH_TRIANGLE) {
        yPos = getYPos(i,spacing);
        for(int j=0;j<=i;j++) {
          float xPos = getTriYPos(j,i,spacing);
          nvgBeginPath(args.vg);
          if(j > 0) {
            nvgMoveTo(args.vg,xPos,yPos);
            nvgLineTo(args.vg,getTriYPos(j-1,i,spacing),yPos);
          }
          if(i > 0 && (j < i || j ==0)) {
            nvgMoveTo(args.vg,xPos,yPos);
            nvgLineTo(args.vg,getTriYPos(j,i-1,spacing),getYPos(i-1,spacing));
          }
          if(i > 0 && (j > 0)) {
            nvgMoveTo(args.vg,xPos,yPos);
            nvgLineTo(args.vg,getTriYPos(j-1,i-1,spacing),getYPos(i-1,spacing));
          }
          nvgStroke(args.vg);
        }      
      } else if(module->meshTopology == module->MESH_CUBIC) {
        yPos = getYPos(i,spacing3D);
        for(int j=0;j<module->meshSize;j++) {
          for(int k=0;k<module->meshSize;k++) {
            yPos = getCubicYPos(i,k,spacing3D);
            float xPos = getCubicXPos(j,k,spacing3D);
            nvgBeginPath(args.vg);
            if(j > 0) {
              nvgMoveTo(args.vg,xPos,yPos);
              nvgLineTo(args.vg,getCubicXPos(j-1,k,spacing3D),yPos);
            }
            if(i > 0) {
              nvgMoveTo(args.vg,xPos,yPos);
              nvgLineTo(args.vg,xPos,getCubicYPos(i-1,k,spacing3D));
            }
            if(k > 0) {
              nvgMoveTo(args.vg,xPos,yPos);
              nvgLineTo(args.vg,getCubicXPos(j,k-1,spacing3D),getCubicYPos(i,k-1,spacing3D));
            }
            nvgStroke(args.vg);
          }
        }
      }
    }


    //Draw dots
    
    for(int i=0;i<module->meshSize;i++) {
      float yPos;
      if(module->meshTopology == module->MESH_RECTILINEAR) {
        yPos = getYPos(i,spacing);
        for(int j=0;j<module->meshSize;j++) {
          float xPos = getRectXPos(j,spacing);
          nvgBeginPath(args.vg);
          nvgCircle(args.vg,xPos,yPos,z);
          nvgFill(args.vg);
        }
      } else if(module->meshTopology == module->MESH_TRIANGLE)  {
        yPos = getYPos(i,spacing);
        for(int j=0;j<=i;j++) {
          float xPos = getTriYPos(j,i,spacing);
          nvgBeginPath(args.vg);
          nvgCircle(args.vg,xPos,yPos,z);
          nvgFill(args.vg);
        }
      } else if(module->meshTopology == module->MESH_CUBIC) {
        for(int j=0;j<module->meshSize;j++) {
          for(int k=0;k<module->meshSize;k++) {
            yPos = getCubicYPos(i,k,spacing3D);
            float xPos = getCubicXPos(j,k,spacing3D);
            nvgBeginPath(args.vg);
            nvgCircle(args.vg,xPos,yPos,(1.0 - (float(k) / meshSizef)) + 1.0);
            nvgFill(args.vg);
          }
        }
      }
    }      

  }


};




struct DanceThisMeshAroundWidget : ModuleWidget {
  
  DanceThisMeshAroundWidget(DanceThisMeshAroundModule *module) {
    setModule(module);
    box.size = Vec(30 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT);

    setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/DanceThisMeshAround.svg")));


    {
      DMDisplayFilterResponse *dfr = new DMDisplayFilterResponse();    
      //if (module) {
        dfr->module = module;
      //}   
      dfr->box.pos = Vec(10.5, 23.5);
      dfr->box.size = Vec(128, 100);
      addChild(dfr);
    }

    {
      DMDisplayMesh *dc = new DMDisplayMesh();    
      //if (module) {
        dc->module = module;
      //}   
      dc->box.pos = Vec(158.5, 23.5);
      dc->box.size = Vec(100, 100);
      addChild(dc);
    }


    

    addParam(createParam<LightSmallKnobSnap>(Vec(170, 133), module, DanceThisMeshAroundModule::MESH_TOPOLOGY_PARAM));
    {
      SmallArcDisplay *c = new SmallArcDisplay();
      if (module) {
        c->percentage = &module->topologyPercentage;
      }
      c->box.pos = Vec(173.5, 136.5);
      c->box.size = Vec(30, 30);
      addChild(c);
    }


    addParam(createParam<LightSmallKnobSnap>(Vec(220, 133), module, DanceThisMeshAroundModule::MESH_SIZE_PARAM));
    {
      SmallArcDisplay *c = new SmallArcDisplay();
      if (module) {
        c->percentage = &module->meshSizePercentage;
      }
      c->box.pos = Vec(223.5, 136.5);
      c->box.size = Vec(30, 30);
      addChild(c);
    }
    addInput(createInput<LightPort>(Vec(236, 145), module, DanceThisMeshAroundModule::MESH_SIZE_INPUT));


    addParam(createParam<LightSmallKnob>(Vec(10, 133), module, DanceThisMeshAroundModule::BP_1_CUTOFF_PARAM));
    {
      SmallArcDisplay *c = new SmallArcDisplay();
      if (module) {
        c->percentage = &module->bpCutoff1Percentage;
      }
      c->box.pos = Vec(13.5, 136.5);
      c->box.size = Vec(30, 30);
      addChild(c);
    }  
    addInput(createInput<LightPort>(Vec(26, 145), module, DanceThisMeshAroundModule::BP_1_CUTOFF_INPUT));

    addParam(createParam<LightSmallKnob>(Vec(55, 133), module, DanceThisMeshAroundModule::BP_2_CUTOFF_PARAM));
    {
      SmallArcDisplay *c = new SmallArcDisplay();
      if (module) {
        c->percentage = &module->bpCutoff2Percentage;
      }
      c->box.pos = Vec(58.5, 136.5);
      c->box.size = Vec(30, 30);
      addChild(c);
    }
    addInput(createInput<LightPort>(Vec(71, 145), module, DanceThisMeshAroundModule::BP_2_CUTOFF_INPUT));

    addParam(createParam<LightSmallKnob>(Vec(100, 133), module, DanceThisMeshAroundModule::BP_3_CUTOFF_PARAM));
    {
      SmallArcDisplay *c = new SmallArcDisplay();
      if (module) {
        c->percentage = &module->bpCutoffMeshPercentage;
      }
      c->box.pos = Vec(103.5, 136.5);
      c->box.size = Vec(30, 30);
      addChild(c);
    }
    addInput(createInput<LightPort>(Vec(116, 145), module, DanceThisMeshAroundModule::BP_3_CUTOFF_INPUT));


    addParam(createParam<LightSmallKnob>(Vec(10, 253), module, DanceThisMeshAroundModule::GROUP_FEEDBACK_PARAM));
    {
      SmallArcDisplay *c = new SmallArcDisplay();
      if (module) {
        c->percentage = &module->groupFeedbackPercentage;
      }
      c->box.pos = Vec(13.5, 256.5);
      c->box.size = Vec(30, 30);
      addChild(c);
    }  
    addInput(createInput<LightPort>(Vec(26, 265), module, DanceThisMeshAroundModule::GROUP_FEEDBACK_INPUT));

    addParam(createParam<LightSmallKnob>(Vec(55, 253), module, DanceThisMeshAroundModule::INPUT_NONLIARITY_PARAM));
    {
      SmallArcDisplay *c = new SmallArcDisplay();
      if (module) {
        c->percentage = &module->inputNonlinerityPercentage;
      }
      c->box.pos = Vec(58.5, 256.5);
      c->box.size = Vec(30, 30);
      addChild(c);
    }
    addInput(createInput<LightPort>(Vec(71, 265), module, DanceThisMeshAroundModule::INPUT_NONLIARITY_INPUT));



    addParam(createParam<LightSmallKnobSnap>(Vec(10, 193), module, DanceThisMeshAroundModule::DELAY_TIME_1_PARAM));
    {
      SmallArcDisplay *c = new SmallArcDisplay();
      if (module) {
        c->percentage = &module->delayTime1Percentage;
      }
      c->box.pos = Vec(13.5, 196.5);
      c->box.size = Vec(30, 30);
      addChild(c);
    }
    addInput(createInput<LightPort>(Vec(26, 205), module, DanceThisMeshAroundModule::DELAY_TIME_1_INPUT));

    addParam(createParam<LightSmallKnobSnap>(Vec(55, 193), module, DanceThisMeshAroundModule::DELAY_TIME_2_PARAM));
    {
      SmallArcDisplay *c = new SmallArcDisplay();
      if (module) {
        c->percentage = &module->delayTime2Percentage;
      }
      c->box.pos = Vec(58.5, 196.5);
      c->box.size = Vec(30, 30);
      addChild(c);
    }
    addInput(createInput<LightPort>(Vec(71, 205), module, DanceThisMeshAroundModule::DELAY_TIME_2_INPUT));


    addParam(createParam<LightSmallKnobSnap>(Vec(145, 193), module, DanceThisMeshAroundModule::DELAY_TIME_MESH_PARAM));
    {
      SmallArcDisplay *c = new SmallArcDisplay();
      if (module) {
        c->percentage = &module->delayTimeMeshPercentage;
      }
      c->box.pos = Vec(148.5, 196.5);
      c->box.size = Vec(30, 30);
      addChild(c);
    }
    addInput(createInput<LightPort>(Vec(161, 205), module, DanceThisMeshAroundModule::DELAY_TIME_MESH_INPUT));


    addParam(createParam<LightSmallKnob>(Vec(185, 193), module, DanceThisMeshAroundModule::DELAY_MESH_FB_AMOUNT_PARAM));
    {
      SmallArcDisplay *c = new SmallArcDisplay();
      if (module) {
        c->percentage = &module->delayTimeMeshFeedbackPercentage;
      }
      c->box.pos = Vec(188.5, 196.5);
      c->box.size = Vec(30, 30);
      addChild(c);
    }
    addInput(createInput<LightPort>(Vec(201, 205), module, DanceThisMeshAroundModule::DELAY_MESH_FB_AMOUNT_INPUT));

    addParam(createParam<LightSmallKnob>(Vec(225, 193), module, DanceThisMeshAroundModule::DELAY_MESH_FB_NONLINEARITY_PARAM));
    {
      SmallArcDisplay *c = new SmallArcDisplay();
      if (module) {
        c->percentage = &module->delayTimeMeshNonLinearityPercentage;
      }
      c->box.pos = Vec(228.5, 196.5);
      c->box.size = Vec(30, 30);
      addChild(c);
    }
    addInput(createInput<LightPort>(Vec(241, 205), module, DanceThisMeshAroundModule::DELAY_MESH_FB_NONLINEARITY_INPUT));



    // addParam(createParam<LightSmallKnob>(Vec(225, 243), module, DanceThisMeshAroundModule::ALLPASS_FC_PARAM));
    // // {
    // //   SmallArcDisplay *c = new SmallArcDisplay();
    // //   if (module) {
    // //     c->percentage = &module->delayTimeMeshNonLinearityPercentage;
    // //   }
    // //   c->box.pos = Vec(228.5, 246.5);
    // //   c->box.size = Vec(30, 30);
    // //   addChild(c);
    // // }
    // addInput(createInput<LightPort>(Vec(241, 255), module, DanceThisMeshAroundModule::ALLPASS_FC_INPUT));



  

    // addParam(createParam<RecButton>(Vec(176, 182), module, DanceThisMeshAroundModule::SYNC_MODE_PARAM));
    // addChild(createLight<LargeSMLight<RectangleLight<RedGreenBlueLight>>>(Vec(178, 183), module, DanceThisMeshAroundModule::SYNC_MODE_LIGHT));

    // addParam(createParam<RecButton>(Vec(8, 277), module, DanceThisMeshAroundModule::MORPH_MODE_PARAM));
    // addChild(createLight<LargeSMLight<RectangleLight<RedGreenBlueLight>>>(Vec(10, 278), module, DanceThisMeshAroundModule::MORPH_MODE_LIGHT));


    // addInput(createInput<LightPort>(Vec(48, 127), module, DanceThisMeshAroundModule::V_OCTAVE_INPUT));
    // addInput(createInput<LightPort>(Vec(5, 166), module, DanceThisMeshAroundModule::FM_INPUT));
    // addInput(createInput<LightPort>(Vec(32, 166), module, DanceThisMeshAroundModule::SYNC_INPUT));
    // addInput(createInput<LightPort>(Vec(59, 166), module, DanceThisMeshAroundModule::PHASE_INPUT));

    addInput(createInput<LightPort>(Vec(10, 304), module, DanceThisMeshAroundModule::IMPULSE_INPUT));
    addOutput(createOutput<LightPort>(Vec(228, 304), module, DanceThisMeshAroundModule::OUTPUT_1));

      
    addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH-12, 0)));
    addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH + 12, 0)));
    addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH-12, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
    addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH + 12, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

  }

};

Model *modelDanceThisMeshAround = createModel<DanceThisMeshAroundModule, DanceThisMeshAroundWidget>("DanceThisMeshAround");
