#include "../controller/BoxOfRevelation.hpp"

#include "../component/knob.hpp"
#include "../component/light.hpp"
#include "../component/port.hpp"
#include "../component/button.hpp"
#include "../component/display.hpp"
#include "../component/menu.hpp"


struct BRLoadModelCubeItem : MenuItem {
	BoxOfRevelationModule *hsm ;
  void onAction(const event::Action &e) override {
		
		//char *path = osdialog_file(OSDIALOG_OPEN, NULL, NULL, NULL);        //////////dir.c_str(),
		char *path = osdialog_file(OSDIALOG_OPEN, NULL, NULL, NULL);        //////////dir.c_str(),
		if (path) {
			hsm->loadCubeFile(path);
			free(path);
		}
	}
};


struct BRDisplayFilterResponse : FramebufferWidget {
  BoxOfRevelationModule *module;

  BRDisplayFilterResponse() {
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
        float xPosition = (std::log10(i) * 87.18);
        nvgMoveTo(args.vg,xPosition,0.5);
        nvgLineTo(args.vg,xPosition,100.5);
    }
    for(int i=10;i<100;i+=10) {
        float xPosition = (std::log10(i) * 87.18);
        nvgMoveTo(args.vg,xPosition,0.5);
        nvgLineTo(args.vg,xPosition,100.5);
    }
    float xPosition = (std::log10(150) * 87.18);
    nvgMoveTo(args.vg,xPosition,0.5);
    nvgLineTo(args.vg,xPosition,100.5);
    nvgStroke(args.vg);

    for(int i=1;i<6;i++) {
      nvgBeginPath(args.vg);
      nvgStrokeColor(args.vg, i !=3 ? nvgRGB(0x50,0x50,0x50) : nvgRGB(0xa0,0xa0,0xa0));
      nvgMoveTo(args.vg,0.5,i*16);
      nvgLineTo(args.vg,204.5,i*16);
      nvgStroke(args.vg);
    }

    if (!module || module->nbrCubeModels == 0 || module->currentModel == -1) 
      return;

        //fprintf(stderr, "Point x:%i l:%i freq:%f response:%f  gain: %f  \n",x,l,frequency,response,levelResponse);

    float driveLevel = 0.0;
    float filtersInLevel = 0;
    for(int s=0;s<NBR_FILTER_STAGES;s++) {
        if(module->cubeModels[module->currentModel].filterLevel[s] >= 0 && module->cubeModels[module->currentModel].filterNonlinearityStructure[s] != 0) {
          driveLevel += module->drive[s]; 
          filtersInLevel +=1.0;
        }
    }
    if(filtersInLevel > 0) {
      driveLevel = clamp(driveLevel/filtersInLevel,0.1,5.0);
    }

//driveLevel = 0.0;
    float r = interpolate(176.0f,255.0f,driveLevel,0.0f,5.0f);
    float g = interpolate(176.0f,0.0f,driveLevel,0.0f,5.0f);
    float b = interpolate(255.0f,0.0f,driveLevel,0.0f,5.0f);

        // fprintf(stderr, "drive level: %f  r:%f g:%f b:%f \n",driveLevel,r,g,b);

    nvgBeginPath(args.vg);  
    //nvgStrokeColor(args.vg, nvgRGB(0xb0,0xb0,0xff));
    nvgStrokeColor(args.vg, nvgRGB(r,g,b));
    nvgStrokeWidth(args.vg, 1);
    for(int x=0;x<204;x+=1) {
      double frequency = std::pow(10,x/87.18f + 2.0f) / module->sampleRate;
      double response = 1;
      
      for(int l=0;l<module->nbrfilterLevels;l++) {
        int filtersInLevel = 0;
        double levelResponse = 0;
        for(int s=0;s<NBR_FILTER_STAGES;s++) {
            if(module->cubeModels[module->currentModel].filterLevel[s] == l) {
              levelResponse += module->pFilter[s][0]->frequencyResponse(frequency) * module->attenuation[s]; 
              filtersInLevel +=1;
        // fprintf(stderr, "Point x:%i l:%i freq:%f response:%f  level response: %f  \n",x,l,frequency[0],response[0],levelResponse[0]);
            }
        }
        if(filtersInLevel > 0) {
          levelResponse = levelResponse / std::sqrt(filtersInLevel);
          response =  response * levelResponse;
        }
      }
      //attenuation[s] = powf(10,_gain / 20.0f);
      response = std::log10(response * module->makeupAttenuation) * 20 + 50;
      response = clamp(response,0.0,100);
        // fprintf(stderr, "Point x:%i response:%f  \n",x,response[0]);

      if(x < 1)
        nvgMoveTo(args.vg,0,100.0 - response);
      else  
        nvgLineTo(args.vg,x,100.0 - response);
    
    }
    nvgStroke(args.vg);


  }

};  

struct BRDisplayCube : FramebufferWidget {
  BoxOfRevelationModule *module;
  std::shared_ptr<Font> font;

  BRDisplayCube() {
    font = APP->window->loadFont(asset::plugin(pluginInstance, "res/fonts/routed-gothic.ttf"));
  }


  void draw(const DrawArgs &args) override {
    //background
    nvgFillColor(args.vg, nvgRGB(20, 30, 33));
    nvgBeginPath(args.vg);
    nvgRect(args.vg, 0, 0, box.size.x, box.size.y);
    nvgFill(args.vg);


    if (!module) 
      return;


    float boxEdgeLength = 72.0; 
    float boxXOffset = 10;
    float boxYOffset = 45;
    nvgBeginPath(args.vg);
    nvgStrokeWidth(args.vg, 1);
    nvgStrokeColor(args.vg, nvgRGB(0xb0,0xb0,0x2f));

    for(int i=0;i<12;i++) {

        int vOrigin = module->edges[i][0];
        float x = module->cubePoints[vOrigin][0];
        float y = module->cubePoints[vOrigin][1];
        float z = module->cubePoints[vOrigin][2];
        float xPrimeO =  (x + z/2) * boxEdgeLength + boxXOffset;
        float yPrimeO =  (y - z/2) * boxEdgeLength + boxYOffset;
               //fprintf(stderr, "FROM i:%i x:%f y:%f z:%f xp:%f yp:%f \n",i,x,y,z,xPrimeO,yPrimeO);
        nvgMoveTo(args.vg,xPrimeO,yPrimeO);

        int vDest = module->edges[i][1];
        x = module->cubePoints[vDest][0];
        y = module->cubePoints[vDest][1];
        z = module->cubePoints[vDest][2];
        float xPrimeD =  (x + z/2) * boxEdgeLength + boxXOffset;
        float yPrimeD =  (y - z/2) * boxEdgeLength + boxYOffset;
               //fprintf(stderr, "TO i:%i x:%f y:%f z:%f xp:%f yp:%f \n",i,x,y,z,xPrimeD,yPrimeD);
        nvgLineTo(args.vg,xPrimeD,yPrimeD);

    }
    nvgStroke(args.vg);


    float x = module->currentPoint.x;
    float y = module->currentPoint.y;
    float z = module->currentPoint.z;
    float xPrimeC =  (x + z/2) * boxEdgeLength + boxXOffset;
    float yPrimeC =  ((1.0 - (y + z/2)) * boxEdgeLength) + boxYOffset;


    nvgFillColor(args.vg,nvgRGB(0x1f,0xd0,0x1f)); 	
    nvgBeginPath(args.vg);
    nvgCircle(args.vg,xPrimeC,yPrimeC,6-(3*z));
    nvgFill(args.vg);
  }
};



struct BRDisplayModelName : FramebufferWidget {
  BoxOfRevelationModule *module;
  std::shared_ptr<Font> font;

  BRDisplayModelName() {
    font = APP->window->loadFont(asset::plugin(pluginInstance, "res/fonts/routed-gothic.ttf"));
  }

  void draw(const DrawArgs &args) override {
    //background
    nvgFillColor(args.vg, nvgRGB(20, 30, 33));
    nvgBeginPath(args.vg);
    nvgRect(args.vg, 0, 0, box.size.x, box.size.y);
    nvgFill(args.vg);

    if (!module || module->nbrCubeModels == 0 || module->currentModel == -1) 
      return;

    nvgFontSize(args.vg, 18);
    nvgFontFaceId(args.vg, font->handle);
    nvgTextLetterSpacing(args.vg, -0.5);
    nvgFillColor(args.vg,nvgRGB(0x1f,0xf0,0x1f)); 	
    char text[128];
    snprintf(text, sizeof(text), "%s", module->cubeModels[module->currentModel].modelName.c_str()); // needs to use model #
    nvgText(args.vg, 2, 14, text, NULL);      
  }
};




struct BoxOfRevelationWidget : ModuleWidget {
  
  BoxOfRevelationWidget(BoxOfRevelationModule *module) {
    setModule(module);
    box.size = Vec(30 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT);

    setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/BoxOfRevelation.svg")));


    {
      BRDisplayFilterResponse *dfr = new BRDisplayFilterResponse();    
      //if (module) {
        dfr->module = module;
      //}   
      dfr->box.pos = Vec(10.5, 23.5);
      dfr->box.size = Vec(204, 100);
      addChild(dfr);
    }

    {
      BRDisplayCube *dc = new BRDisplayCube();    
      //if (module) {
        dc->module = module;
      //}   
      dc->box.pos = Vec(29.5, 164.5);
      dc->box.size = Vec(128, 128);
      addChild(dc);
    }

    // {
    //   BCDisplayWaveFiles *dwf = new BCDisplayWaveFiles();    
    //   //if (module) {
    //     dwf->module = module;
    //   //}   
    //   dwf->box.pos = Vec(86.5, 172.5);
    //   dwf->box.size = Vec(76, 32);
    //   addChild(dwf);
    // }

    {
      BRDisplayModelName *dmn = new BRDisplayModelName();    
      //if (module) {
        dmn->module = module;
      //}       
      dmn->box.pos = Vec(29.5, 132.5);
      dmn->box.size = Vec(185, 24);
      addChild(dmn);
    }


    addParam(createParam<LightKnobSnap>(Vec(168, 172), module, BoxOfRevelationModule::FILTER_MODEL_PARAM));
    {
      ArcDisplay *c = new ArcDisplay();
      if (module) {
        c->percentage = &module->modelPercentage;
      }
      c->box.pos = Vec(175, 178);
      c->box.size = Vec(60, 60);
      addChild(c);
    }
    addInput(createInput<LightPort>(Vec(198, 204), module, BoxOfRevelationModule::FILTER_MODEL_INPUT));

    

    addParam(createParam<LightSmallKnob>(Vec(24.5, 302), module, BoxOfRevelationModule::FREQUENCY_PARAM));
    {
      SmallArcDisplay *c = new SmallArcDisplay();
      if (module) {
        c->percentage = &module->frequencyPercentage;
      }
      c->box.pos = Vec(28, 305.5);
      c->box.size = Vec(30, 30);    
      addChild(c);
    }
    addInput(createInput<LightPort>(Vec(43.5, 300), module, BoxOfRevelationModule::FREQUENCY_INPUT));


    addParam(createParam<LightSmallKnob>(Vec(74.5, 302), module, BoxOfRevelationModule::Y_PARAM));
    {
      SmallArcDisplay *c = new SmallArcDisplay();
      if (module) {
        c->percentage = &module->yMorphPercentage;
      }
      c->box.pos = Vec(78, 305.5);
      c->box.size = Vec(30, 30);
      addChild(c);
    }
    addInput(createInput<LightPort>(Vec(93.5, 300), module, BoxOfRevelationModule::Y_INPUT));


    addParam(createParam<LightSmallKnob>(Vec(123, 302), module, BoxOfRevelationModule::Z_PARAM));
    {
      SmallArcDisplay *c = new SmallArcDisplay();
      if (module) {
        c->percentage = &module->zMorphPercentage;
      }
      c->box.pos = Vec(126.5, 305.5);
      c->box.size = Vec(30, 30);
      addChild(c);
    }
    addInput(createInput<LightPort>(Vec(142, 300), module, BoxOfRevelationModule::Z_INPUT));


    addInput(createInput<LightPort>(Vec(92, 342), module, BoxOfRevelationModule::INPUT_L));
    addInput(createInput<LightPort>(Vec(114, 342), module, BoxOfRevelationModule::INPUT_R));


    addOutput(createOutput<LightPort>(Vec(162, 342), module, BoxOfRevelationModule::OUTPUT_L));
    addOutput(createOutput<LightPort>(Vec(184, 342), module, BoxOfRevelationModule::OUTPUT_R));



      
    addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH-12, 0)));
    addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH + 12, 0)));
    addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH-12, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
    addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH + 12, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

  }

  void appendContextMenu(Menu* menu) override {
		BoxOfRevelationModule* bor = dynamic_cast<BoxOfRevelationModule*>(this->module);
		assert(bor);

		menu->addChild(new MenuLabel());

    BRLoadModelCubeItem *loadModelCube = new BRLoadModelCubeItem;
    loadModelCube->text = "Load Model Cube File";
    loadModelCube->hsm = bor;
    menu->addChild(loadModelCube);
    
  }


};

Model *modelBoxOfRevelation = createModel<BoxOfRevelationModule, BoxOfRevelationWidget>("BoxOfRevelation");
