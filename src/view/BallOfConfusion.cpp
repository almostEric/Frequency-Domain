#include "../controller/BallOfConfusion.hpp"

#include "../component/knob.hpp"
#include "../component/light.hpp"
#include "../component/port.hpp"
#include "../component/button.hpp"
#include "../component/display.hpp"
#include "../component/menu.hpp"



struct BCDirPLAYERItem : MenuItem {
	BallOfConfusionModule *hsm ;
	void onAction(const event::Action &e) override {
		
		char *path = osdialog_file(OSDIALOG_OPEN, NULL, NULL, NULL);        //////////dir.c_str(),
		if (path) {
			hsm->loadDirectory(path);
			free(path);
		}
	}
};


struct BCPLAYERItem : MenuItem {
	BallOfConfusionModule *hsm ;
  void onAction(const event::Action &e) override {
		
		char *path = osdialog_file(OSDIALOG_OPEN, NULL, NULL, NULL);        //////////dir.c_str(),
		if (path) {
			hsm->loadIndividualWavefile(path);
			free(path);
		}
	}
};

struct BCPLAYERAddItem : MenuItem {
	BallOfConfusionModule *hsm ;
  void onAction(const event::Action &e) override {
		
		char *path = osdialog_file(OSDIALOG_OPEN, NULL, NULL, NULL);        //////////dir.c_str(),
		if (path) {
			hsm->loadAdditionalWavefile(path);
			free(path);
		}
	}
};

struct ScatterSlider : ui::Slider {
			struct ScatterQuantity : Quantity {
				BallOfConfusionModule* module;
				const float MAX = 1.f;

				ScatterQuantity(BallOfConfusionModule* module) {
					this->module = module;
				}
				void setValue(float value) override {
					module->scatterPercent = math::clamp(value * MAX, 0.f, MAX);
				}
				float getValue() override {
					return module->scatterPercent;
				}
				float getDefaultValue() override {
					return 1.0;
				}
				float getDisplayValue() override {
					return getValue() * 100;
				}
				void setDisplayValue(float displayValue) override {
					setValue(displayValue / (100 * MAX));
				}
				std::string getLabel() override {
					return "Sample Scatter";
				}
				std::string getUnit() override {
					return "%";
				}
			};

			ScatterSlider(BallOfConfusionModule* module) {
				this->box.size.x = 200.0;
				quantity = new ScatterQuantity(module);
			}
			~ScatterSlider() {
				delete quantity;
			}
		};


struct BCDisplayWaveform : FramebufferWidget {
  BallOfConfusionModule *module;

  BCDisplayWaveform() {
  }


  void draw(const DrawArgs &args) override {
    //background
    nvgFillColor(args.vg, nvgRGB(20, 30, 33));
    nvgBeginPath(args.vg);
    nvgRect(args.vg, 0, 0, box.size.x, box.size.y);
    nvgFill(args.vg);

    if (!module) 
      return;

    if(module->syncMode && module->syncPosition < 1) {
      nvgBeginPath(args.vg);
      nvgStrokeColor(args.vg, nvgRGB(0xf0,0xb0,0x1f));
      nvgStrokeWidth(args.vg, 0.5);
      nvgMoveTo(args.vg,module->syncPosition*204.0,0);
      nvgLineTo(args.vg,204*module->syncPosition,100);
      nvgStroke(args.vg);
    }

    nvgBeginPath(args.vg);
    nvgStrokeColor(args.vg, nvgRGB(0xb0,0xb0,0xff));
    nvgStrokeWidth(args.vg, 1);
    float runningTotal = 0;
    float x =0;
    for(int i=0;i<2048;i++) {
        runningTotal += module->actualWaveTable[i]; 
        if(i % 10 == 9) {
          float y = runningTotal / 10.0;
          if(x < 1)
            nvgMoveTo(args.vg,0,y*50.0+50.0);
          else  
            nvgLineTo(args.vg,x,y*50.0+50.0);
          runningTotal = 0.0;
          x++;
        }
    }
    nvgStroke(args.vg);

  }

};

struct BCDisplaySphere : FramebufferWidget {
  BallOfConfusionModule *module;
  std::shared_ptr<Font> font;

  BCDisplaySphere() {
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

//        fprintf(stderr, "histogram! \n");

      nvgFontSize(args.vg, 9);
      nvgFontFaceId(args.vg, font->handle);
      nvgTextLetterSpacing(args.vg, -0.5);
      nvgFillColor(args.vg,nvgRGB(0xd0,0xd0,0x1f)); 	
      char text[128];
      snprintf(text, sizeof(text), "#: %llu", module->sphere.size());
      nvgTextAlign(args.vg,NVG_ALIGN_LEFT);
      nvgText(args.vg, 2, box.size.y-1, text, NULL);



    nvgStrokeWidth(args.vg, 0.5);
    

    for(uint64_t i=0;i<module->sphere.size();i++) {
        float x = module->sphere[i].xRotated; 
        float y = module->sphere[i].yRotated;
        float z = module->sphere[i].zRotated + 2; //pushing it back so no non-zero
        uint16_t fileId = module->sphere[i].fileId;

        float f = 1.5;
        float sphereRadius = 68.0; //Probably needs to be smaller

        float xPrime =  x * f/z * sphereRadius + sphereRadius - 4.0;
        float yPrime =  y * f/z * sphereRadius + sphereRadius - 4.0;

        bool wtInUse = false;
        uint64_t usingIndex;
        for(uint64_t j=0;j<4;j++) {
          if(module->waveTablesInUse[j] == i) {
            usingIndex = j;
            wtInUse = true;
            break;
          }
        }
        nvgFillColor(args.vg,wtInUse ? nvgRGB(0xf0-(0xc0 * module->waveTableWeighting[usingIndex]),0xff,0x1f) : nvgRGB(0xd0,0xd0,0x1f)); 	
        float qr =(fileId % 12) * 20.0; 
        float qg =(fileId % 3) * 80.0; 
        float qb =(255.0f -(fileId % 5) * 50.0); 
        nvgStrokeColor(args.vg, nvgRGB(qr,qg,qb));
        //nvgStrokeColor(args.vg, nvgRGB((fileId % 12) * 10,0xff,0xff));

       //fprintf(stderr, "file id %u \n",(fileId % 12) * 10);
        nvgBeginPath(args.vg);
        nvgCircle(args.vg,xPrime,yPrime,f/z);
        nvgFill(args.vg);
        nvgClosePath(args.vg);
        nvgStroke(args.vg);
    }
  }
};

struct BCDisplayWaveFiles : FramebufferWidget {
  BallOfConfusionModule *module;
  std::shared_ptr<Font> font;

  BCDisplayWaveFiles() {
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

//        fprintf(stderr, "histogram! \n");

    if(module->sphere.size() > 0) {

      nvgFontSize(args.vg, 9);
      nvgFontFaceId(args.vg, font->handle);
      nvgTextLetterSpacing(args.vg, -0.5);


      for(uint64_t j=0;j<4;j++) {
        if(module->waveTableWeighting[j] > 0) {
          nvgFillColor(args.vg,nvgRGB(0xf0-(0xc0 * module->waveTableWeighting[j]),0xff,0x1f)); 	
          char text[128];
          snprintf(text, sizeof(text), "%s", module->waveTableNames[module->waveTablesInUse[j]].c_str());
          nvgTextAlign(args.vg,NVG_ALIGN_LEFT);
          nvgText(args.vg, 2, j*7.2+7, text, NULL);

          snprintf(text, sizeof(text), "%.1f", module->waveTableWeighting[j] * 100.0 );
          nvgTextAlign(args.vg,NVG_ALIGN_RIGHT);
          nvgText(args.vg, 74, j*7.2+7, text, NULL);
        }
      }
      
    }
  }
};

struct BCDisplayMorphMode : FramebufferWidget {
  BallOfConfusionModule *module;
  std::shared_ptr<Font> font;

  BCDisplayMorphMode() {
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

    nvgFontSize(args.vg, 9);
    nvgFontFaceId(args.vg, font->handle);
    nvgTextLetterSpacing(args.vg, -0.5);
    nvgFillColor(args.vg,nvgRGB(0x1f,0xf0,0x1f)); 	
    char text[128];
    snprintf(text, sizeof(text), "%s", module->morphModes[module->morphMode].c_str());
    nvgText(args.vg, 2, 7, text, NULL);      
  }
};

struct BCDisplaySyncMode : FramebufferWidget {
  BallOfConfusionModule *module;
  std::shared_ptr<Font> font;

  BCDisplaySyncMode() {
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

    nvgFontSize(args.vg, 9);
    nvgFontFaceId(args.vg, font->handle);
    nvgTextLetterSpacing(args.vg, -0.5);
    nvgFillColor(args.vg,nvgRGB(0x1f,0xf0,0x1f)); 	
    char text[128];
    snprintf(text, sizeof(text), "%s", module->syncModes[module->syncMode ? 1 : 0].c_str());
    nvgText(args.vg, 2, 7, text, NULL);      
  }
};




struct BallOfConfusionWidget : ModuleWidget {
  
  BallOfConfusionWidget(BallOfConfusionModule *module) {
    setModule(module);
    box.size = Vec(30 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT);

    setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/BallOfConfusion.svg")));


    {
      BCDisplayWaveform *dh = new BCDisplayWaveform();    
      //if (module) {
        dh->module = module;
      //}   
      dh->box.pos = Vec(10.5, 23.5);
      dh->box.size = Vec(204, 100);
      addChild(dh);
    }

    {
      BCDisplaySphere *dh = new BCDisplaySphere();    
      //if (module) {
        dh->module = module;
      //}   
      dh->box.pos = Vec(86.5, 212.5);
      dh->box.size = Vec(128, 128);
      addChild(dh);
    }

    {
      BCDisplayWaveFiles *dwf = new BCDisplayWaveFiles();    
      //if (module) {
        dwf->module = module;
      //}   
      dwf->box.pos = Vec(86.5, 172.5);
      dwf->box.size = Vec(76, 32);
      addChild(dwf);
    }

    {
      BCDisplayMorphMode *dmm = new BCDisplayMorphMode();    
      //if (module) {
        dmm->module = module;
      //}   
      dmm->box.pos = Vec(8, 258);
      dmm->box.size = Vec(32, 10);
      addChild(dmm);
    }

    {
      BCDisplaySyncMode *dsm = new BCDisplaySyncMode();    
      //if (module) {
        dsm->module = module;
      //}   
      dsm->box.pos = Vec(182, 168);
      dsm->box.size = Vec(22, 10);
      addChild(dsm);
    }

    

    addParam(createParam<LightSmallKnob>(Vec(81, 346), module, BallOfConfusionModule::YAW_PARAM));
    {
      SmallBidirectionalArcDisplay *c = new SmallBidirectionalArcDisplay();
      if (module) {
        c->percentage = &module->yawPercentage;
      }
      c->box.pos = Vec(84.5, 349.5);
      c->box.size = Vec(30, 30);
      addChild(c);
    }
    addInput(createInput<LightPort>(Vec(101, 339), module, BallOfConfusionModule::YAW_INPUT));


    addParam(createParam<LightSmallKnob>(Vec(130.5, 346), module, BallOfConfusionModule::PITCH_PARAM));
    {
      SmallBidirectionalArcDisplay *c = new SmallBidirectionalArcDisplay();
      if (module) {
        c->percentage = &module->pitchPercentage;
      }
      c->box.pos = Vec(134, 349.5);
      c->box.size = Vec(30, 30);
      addChild(c);
    }
    addInput(createInput<LightPort>(Vec(150.5, 339), module, BallOfConfusionModule::PITCH_INPUT));


    addParam(createParam<LightSmallKnob>(Vec(179.5, 346), module, BallOfConfusionModule::ROLL_PARAM));
    {
      SmallBidirectionalArcDisplay *c = new SmallBidirectionalArcDisplay();
      if (module) {
        c->percentage = &module->rollPercentage;
      }
      c->box.pos = Vec(183, 349.5);
      c->box.size = Vec(30, 30);
      addChild(c);
    }
    addInput(createInput<LightPort>(Vec(198, 339), module, BallOfConfusionModule::ROLL_INPUT));

  

    addParam(createParam<LightSmallKnob>(Vec(19.5, 129), module, BallOfConfusionModule::FREQUENCY_PARAM));
    {
      SmallArcDisplay *c = new SmallArcDisplay();
      if (module) {
        c->percentage = &module->tuningPercentage;
      }
      c->box.pos = Vec(24, 132.5);
      c->box.size = Vec(30, 30);
      addChild(c);
    }



    addParam(createParam<LightSmallKnob>(Vec(101, 129), module, BallOfConfusionModule::FM_AMOUNT));
    {
      SmallArcDisplay *c = new SmallArcDisplay();
      if (module) {
        c->percentage = &module->fmAmountPercentage;
      }
      c->box.pos = Vec(104.5, 132.5);
      c->box.size = Vec(30, 30);
      addChild(c);
    }
    addInput(createInput<LightPort>(Vec(121, 127), module, BallOfConfusionModule::FM_AMOUNT_INPUT));


    addParam(createParam<LightSmallKnob>(Vec(171, 129), module, BallOfConfusionModule::SYNC_POSITION_PARAM));
    {
      SmallArcDisplay *c = new SmallArcDisplay();
      if (module) {
        c->percentage = &module->syncPositionPercentage;
      }
      c->box.pos = Vec(174.5, 132.5);
      c->box.size = Vec(30, 30);
      addChild(c);
    } 
    addInput(createInput<LightPort>(Vec(191, 127), module, BallOfConfusionModule::SYNC_POSITION_INPUT));


    addParam(createParam<LightSmallKnobSnap>(Vec(45, 268), module, BallOfConfusionModule::SPECTRUM_SHIFT_PARAM));
    {
      SmallBidirectionalArcDisplay *c = new SmallBidirectionalArcDisplay();
      if (module) {
        c->percentage = &module->spectrumShiftPercentage;
      }
      c->box.pos = Vec(48.5, 271.5);
      c->box.size = Vec(30, 30);
      addChild(c);
    }
    addInput(createInput<LightPort>(Vec(60, 250), module, BallOfConfusionModule::SPECTRUM_SHIFT_INPUT));






    addParam(createParam<RecButton>(Vec(178, 180), module, BallOfConfusionModule::SYNC_MODE_PARAM));
    addChild(createLight<LargeSMLight<RectangleLight<RedGreenBlueLight>>>(Vec(180, 181), module, BallOfConfusionModule::SYNC_MODE_LIGHT));

    addParam(createParam<RecButton>(Vec(8, 272), module, BallOfConfusionModule::MORPH_MODE_PARAM));
    addChild(createLight<LargeSMLight<RectangleLight<RedGreenBlueLight>>>(Vec(10, 273), module, BallOfConfusionModule::MORPH_MODE_LIGHT));


    addInput(createInput<LightPort>(Vec(48, 127), module, BallOfConfusionModule::V_OCTAVE_INPUT));
    addInput(createInput<LightPort>(Vec(8, 176), module, BallOfConfusionModule::FM_INPUT));
    addInput(createInput<LightPort>(Vec(44, 176), module, BallOfConfusionModule::SYNC_INPUT));
    addInput(createInput<LightPort>(Vec(14, 208), module, BallOfConfusionModule::PHASE_INPUT));

    addOutput(createOutput<LightPort>(Vec(20, 305), module, BallOfConfusionModule::OUTPUT_L));
    // addOutput(createOutput<LightPort>(Vec(197, 342), module, BallOfConfusionModule::OUTPUT_R));



      
    addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH-12, 0)));
    addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH + 12, 0)));
    addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH-12, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
    addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH + 12, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

  }


  void appendContextMenu(Menu* menu) override {
		BallOfConfusionModule* boc = dynamic_cast<BallOfConfusionModule*>(this->module);
		assert(boc);

		menu->addChild(new MenuLabel());

    //Load Directory
    BCDirPLAYERItem *loadDirectory = new BCDirPLAYERItem;
    loadDirectory->text = "Load Directory";
    loadDirectory->hsm = boc;
    menu->addChild(loadDirectory);


    BCPLAYERItem *singleSample = new BCPLAYERItem;
    singleSample->text = "Load WaveTable";
    singleSample->hsm = boc;
    menu->addChild(singleSample);

    BCPLAYERAddItem *additionalSample = new BCPLAYERAddItem;
    additionalSample->text = "Add WaveTable";
    additionalSample->hsm = boc;
    menu->addChild(additionalSample);

    menu->addChild(new ScatterSlider(boc));    


		// {
    //   OptionsMenuItem* mi = new OptionsMenuItem("Window Function");
		// 	mi->addItem(OptionMenuItem("None", [fs]() { return fs->windowFunctionId == 0; }, [fs]() { fs->windowFunctionId = 0; }));
		// 	mi->addItem(OptionMenuItem("Triangle", [fs]() { return fs->windowFunctionId == 1; }, [fs]() { fs->windowFunctionId = 1; }));
		// 	mi->addItem(OptionMenuItem("Welch", [fs]() { return fs->windowFunctionId == 2; }, [fs]() { fs->windowFunctionId = 2; }));
		// 	mi->addItem(OptionMenuItem("Sine", [fs]() { return fs->windowFunctionId == 3; }, [fs]() { fs->windowFunctionId = 3; }));
		// 	mi->addItem(OptionMenuItem("Hanning", [fs]() { return fs->windowFunctionId == 4; }, [fs]() { fs->windowFunctionId = 4; }));
		// 	mi->addItem(OptionMenuItem("Blackman", [fs]() { return fs->windowFunctionId == 5; }, [fs]() { fs->windowFunctionId = 5; }));
		// 	mi->addItem(OptionMenuItem("Nutall", [fs]() { return fs->windowFunctionId == 6; }, [fs]() { fs->windowFunctionId = 6; }));
		// 	mi->addItem(OptionMenuItem("Kaiser", [fs]() { return fs->windowFunctionId == 7; }, [fs]() { fs->windowFunctionId = 7; }));
		// 	OptionsMenuItem::addToMenu(mi, menu);
		// }
  }
};

Model *modelBallOfConfusion = createModel<BallOfConfusionModule, BallOfConfusionWidget>("BallOfConfusion");
