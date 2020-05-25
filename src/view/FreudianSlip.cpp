#include "../controller/FreudianSlip.hpp"

#include "../component/knob.hpp"
#include "../component/light.hpp"
#include "../component/port.hpp"
#include "../component/button.hpp"
#include "../component/display.hpp"
#include "../component/menu.hpp"


struct PLAYERItem : MenuItem {
	FreudianSlipModule *hsm ;
	void onAction(const event::Action &e) override {
		
		char *path = osdialog_file(OSDIALOG_OPEN, NULL, NULL, NULL);        //////////dir.c_str(),
		if (path) {
			hsm->play = false;
			hsm->reload = true;
			hsm->loadSample(path);
			hsm->samplePos = 0;
			hsm->lastPath = std::string(path);
			free(path);
		}
	}
};

struct DisplayHistogram : FramebufferWidget {
  FreudianSlipModule *module;

  DisplayHistogram() {
  }


  void draw(const DrawArgs &args) override {
    //background
    nvgFillColor(args.vg, nvgRGB(20, 30, 33));
    nvgBeginPath(args.vg);
    nvgRect(args.vg, 0, 0, box.size.x, box.size.y);
    nvgFill(args.vg);

    if (!module) 
      return;
    

    for(uint16_t x=0;x<NUM_UI_FRAMES;x++) {
      for(uint16_t y=0;y<NUM_UI_BANDS;y++) {
        int a = clamp(module->histogram[x][y] * 384,0.0f,255.0f);
        //fprintf(stderr, "%f \n", x);
        nvgFillColor(args.vg, nvgRGBA(0x6a, 0x73, 0x17,a)); //CRT Yellow	
        nvgBeginPath(args.vg);
        nvgRect(args.vg,x,(NUM_UI_BANDS-y)-1,1,1);
        nvgFill(args.vg);
      }
    }
  }
};

struct DisplayPlayStatus : FramebufferWidget {
  std::shared_ptr<Font> font;
  FreudianSlipModule *module;

  DisplayPlayStatus() {
  }


  void draw(const DrawArgs &args) override {

    if (!module)
      return;

    nvgFillColor(args.vg, nvgRGB(0x3a, 0x73, 0x27)); //CRT Green	
    nvgStrokeColor(args.vg, nvgRGB(0x3a, 0x73, 0x27)); //CRT Green	
    float fc = float(module->frameCount);
    if(fc > 0) {
      for(uint8_t i=0;i<MAX_VOICE_COUNT;i++) {
        float x = (module->frameIndex[i]) / fc * 238;
  			nvgBeginPath(args.vg);
        nvgRect(args.vg,x,i*2,2,2);
        nvgFill(args.vg);
      }
    }
  }
};

struct DisplaySampleFileInfo : FramebufferWidget {
  std::shared_ptr<Font> font;
  FreudianSlipModule *module;

  DisplaySampleFileInfo() {

   	font = APP->window->loadFont(asset::plugin(pluginInstance, "res/fonts/DejaVuSansMono.ttf"));

  }


  void draw(const DrawArgs &args) override {
    //background
    nvgFillColor(args.vg, nvgRGB(20, 30, 33));
    nvgBeginPath(args.vg);
    nvgRect(args.vg, 0, 0, box.size.x, box.size.y);
    nvgFill(args.vg);


    if (!module)
      return;

    nvgFontSize(args.vg, 10);
    nvgFontFaceId(args.vg, font->handle);
    nvgTextLetterSpacing(args.vg, -1);
    nvgFillColor(args.vg, nvgRGB(0x3a, 0xa3, 0x27)); //CRT Green	
    nvgTextBox(args.vg, 2, 10,120, module->sampleStatusDesc.c_str(), NULL);
  }
};


struct FreudianSlipWidget : ModuleWidget {
  
  FreudianSlipWidget(FreudianSlipModule *module) {
    setModule(module);
    box.size = Vec(30 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT);

    setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/FreudianSlip.svg")));

    {
      DisplaySampleFileInfo *dsf = new DisplaySampleFileInfo();
      //if (module) {
        dsf->module = module;
      //}
      dsf->box.pos = Vec(5.5, 23.5);
      dsf->box.size = Vec(127, 64);
      addChild(dsf);
    }

    {
      DisplayHistogram *dh = new DisplayHistogram();
      //if (module) {
        dh->module = module;
      //}
      dh->box.pos = Vec(141, 23.5);
      dh->box.size = Vec(238, 64);
      addChild(dh);
    }

    {
      DisplayPlayStatus *dps = new DisplayPlayStatus();
      //if (module) {
        dps->module = module;
      //} 
      dps->box.pos = Vec(141, 23.5);
      dps->box.size = Vec(238, 64);
      addChild(dps);
      
    }


    // VOICE_COUNT
    addInput(createInput<LightPort>(Vec(9, 104), module, FreudianSlipModule::VOICE_COUNT_CV));
    addParam(createParam<LightSmallKnobSnap>(Vec(37.5, 108), module, FreudianSlipModule::VOICE_COUNT));

    {
      SmallArcDisplay *c = new SmallArcDisplay();
      if (module) {
        c->percentage = &module->voiceCountPercentage;
      } 
      c->box.pos = Vec(41, 112);
      c->box.size = Vec(30, 30);
      addChild(c);
    }


    // Waveform
    addInput(createInput<LightPort>(Vec(68, 104), module, FreudianSlipModule::VOICE_WAVEFORM_CV));
    addParam(createParam<LightSmallKnobSnap>(Vec(97, 107), module, FreudianSlipModule::VOICE_WAVEFORM));

    {
      SmallArcDisplay *c = new SmallArcDisplay();
      if (module) {
        c->percentage = &module->waveformPercentage;
      }
      c->box.pos = Vec(100.5, 110.5);
      c->box.size = Vec(30, 30);
      addChild(c);
    }


    // Tuning 
    addInput(createInput<LightPort>(Vec(133, 104), module, FreudianSlipModule::V_OCT_CV));
    addParam(createParam<LightSmallKnob>(Vec(161.5, 108), module, FreudianSlipModule::V_OCT_PARAM));

    {
      SmallBidirectionalArcDisplay *c = new SmallBidirectionalArcDisplay();
      if (module) {
        c->percentage = &module->vOctPercentage;
      }
      c->box.pos = Vec(165, 111.5);
      c->box.size = Vec(30, 30);
      addChild(c);
    }

    //Frequency Wapring
    addInput(createInput<LightPort>(Vec(9, 178), module, FreudianSlipModule::FREQ_WARP_AMOUNT_CV));
    addParam(createParam<LightSmallKnob>(Vec(37.5, 182), module, FreudianSlipModule::FREQ_WARP_AMOUNT));
    {
      SmallArcDisplay *c = new SmallArcDisplay();
      if (module) {
        c->percentage = &module->freqWarpAmount;
      }
      c->box.pos = Vec(41, 185.5);
      c->box.size = Vec(30, 30);
      addChild(c);
    }


    // Warp Center Frequency
    addInput(createInput<LightPort>(Vec(68, 178), module, FreudianSlipModule::FREQ_WARP_CENTER_CV));
    addParam(createParam<LightSmallKnob>(Vec(96.25, 182), module, FreudianSlipModule::FREQ_WARP_CENTER));
    {
      SmallArcDisplay *c = new SmallArcDisplay();
      if (module) {
        c->percentage = &module->freqWarpCenterPercentage;
      }
      c->box.pos = Vec(99.75, 185.5);
      c->box.size = Vec(30, 30);
      addChild(c);
    }
    
    addParam(createParam<RecButton>(Vec(94, 161), module, FreudianSlipModule::FREQ_WARP_USE_FUNDAMENTAL));
    addChild(createLight<LargeSMLight<RectangleLight<RedGreenBlueLight>>>(Vec(96, 162), module, FreudianSlipModule::FREQ_WARP_USE_FUNDAMENTAL_LIGHT));



      // Randomize
    addInput(createInput<LightPort>(Vec(133, 178), module, FreudianSlipModule::RANDOMIZE_CV));
    addParam(createParam<LightSmallKnob>(Vec(161.5, 182), module, FreudianSlipModule::RANDOMIZE_PARAM));
    {
      SmallArcDisplay *c = new SmallArcDisplay();
      if (module) {
        c->percentage = &module->randomizeStepPercentage;
      }
      c->box.pos = Vec(165, 185.5);
      c->box.size = Vec(30, 30);
      addChild(c);
    }


    addParam(createParam<LightSmallKnob>(Vec(247.5, 174), module, FreudianSlipModule::PLAY_SPEED_PARAM));
    {
      SmallBidirectionalArcDisplay *c = new SmallBidirectionalArcDisplay();
      if (module) {
        c->percentage = &module->playSpeedPercentage;
      }
      c->box.pos = Vec(251, 177.5);
      c->box.size = Vec(30, 30);
      addChild(c);
    }

    addParam(createParam<LightSmallKnob>(Vec(141.5, 322), module, FreudianSlipModule::FM_AMOUNT));
    {
      SmallArcDisplay *c = new SmallArcDisplay();
      if (module) {
        c->percentage = &module->fmAmountPercentage;
      }
      c->box.pos = Vec(145, 325.5);
      c->box.size = Vec(30, 30);
      addChild(c);
    }

    addParam(createParam<LightSmallKnob>(Vec(336, 322), module, FreudianSlipModule::RM_MIX));
    {
      SmallArcDisplay *c = new SmallArcDisplay();
      if (module) {
        c->percentage = &module->rmMixPercentage;
      }
      c->box.pos = Vec(339.5, 325.5);
      c->box.size = Vec(30, 30);
      addChild(c);
    }



    //Ring Modulator Enableds 
    // addParam(createParam<RecButton>(Vec(284.5, 320), module, FreudianSlipModule::RING_MODULATION));
    // addChild(createLight<LargeSMLight<RectangleLight<RedGreenBlueLight>>>(Vec(286.5, 321), module, FreudianSlipModule::RING_MODULATION_ENABLED_LIGHT));


    // // Input/Output
    addInput(createInput<LightPort>(Vec(105, 342), module, FreudianSlipModule::PLAY_INPUT));
    // addInput(createInput<LightPort>(Vec(347, 340), module, FreudianSlipModule::INPUT_2));

    addOutput(createOutput<LightPort>(Vec(331, 342), module, FreudianSlipModule::OUTPUT_L));
    addOutput(createOutput<LightPort>(Vec(353, 342), module, FreudianSlipModule::OUTPUT_R));

    addOutput(createOutput<LightPort>(Vec(292, 342), module, FreudianSlipModule::EOC_OUTPUT));


    addParam(createParam<RecButton>(Vec(190, 348), module, FreudianSlipModule::LOOP_PARAM));
    addChild(createLight<LargeSMLight<RectangleLight<RedGreenBlueLight>>>(Vec(192, 349), module, FreudianSlipModule::LOOP_MODE_LIGHT));
    addInput(createInput<LightPort>(Vec(165, 342), module, FreudianSlipModule::LOOP_INPUT));



    // FM Input
    addInput(createInput<LightPort>(Vec(33, 317), module, FreudianSlipModule::FM_INPUT_1));
    addInput(createInput<LightPort>(Vec(55, 317), module, FreudianSlipModule::FM_INPUT_2));

    // FM Input
    addInput(createInput<LightPort>(Vec(232, 317), module, FreudianSlipModule::AM_RM_INPUT_1));
    addInput(createInput<LightPort>(Vec(254, 317), module, FreudianSlipModule::AM_RM_INPUT_2));



    // FM Matrix
    {
      CellGrid *frequencyModulationDisplay = new CellGrid();
      if (module) {
        frequencyModulationDisplay->cells = module->frequencyModulationCells;
        frequencyModulationDisplay->gridName = "FM Matrix";
      }

      frequencyModulationDisplay->box.pos = Vec(24, 243);
      frequencyModulationDisplay->box.size = Vec(64, 64);
      addChild(frequencyModulationDisplay);

      addInput(createInput<LightPort>(Vec(0, 240), module, FreudianSlipModule::FM_SHIFT_X_CV));
      addInput(createInput<LightPort>(Vec(0, 275), module, FreudianSlipModule::FM_SHIFT_Y_CV));
    }

    // FM Amount
    {
      CellBarGrid *FMAmountDisplay = new CellBarGrid();
      if (module) {
        FMAmountDisplay->cells = module->frequencyModulationAmountCells;
        FMAmountDisplay->gridName = "FM Amount";
      }

      FMAmountDisplay->box.pos = Vec(119, 243);
      FMAmountDisplay->box.size = Vec(64, 64);
      addChild(FMAmountDisplay);

      addInput(createInput<LightPort>(Vec(94, 240), module, FreudianSlipModule::FM_AMOUNT_SHIFT_X_CV));
      addInput(createInput<LightPort>(Vec(94, 275), module, FreudianSlipModule::FM_AMOUNT_SHIFT_Y_CV));
    }

    // RM Matrix
    {
      CellGrid *ringModulationDisplay = new CellGrid();
      if (module) {
        ringModulationDisplay->cells = module->ringModulationCells;
        ringModulationDisplay->gridName = "AM/RM Matrix";
      }

      ringModulationDisplay->box.pos = Vec(224, 243);
      ringModulationDisplay->box.size = Vec(64, 64);
      addChild(ringModulationDisplay);

      addInput(createInput<LightPort>(Vec(199, 240), module, FreudianSlipModule::RM_SHIFT_X_CV));
      addInput(createInput<LightPort>(Vec(199, 275), module, FreudianSlipModule::RM_SHIFT_Y_CV));
    }

    // RM Mix
    {
      CellBarGrid *RMMixDisplay = new CellBarGrid(0);
      
      if (module) {
        RMMixDisplay->cells = module->ringModulationMixCells;
        RMMixDisplay->gridName = "AM/RM Mix";
      }

      RMMixDisplay->box.pos = Vec(315, 243);
      RMMixDisplay->box.size = Vec(64, 64);
      addChild(RMMixDisplay);

      addInput(createInput<LightPort>(Vec(290, 240), module, FreudianSlipModule::RM_MIX_SHIFT_X_CV));
      addInput(createInput<LightPort>(Vec(290, 275), module, FreudianSlipModule::RM_MIX_SHIFT_Y_CV));
    }



    // Panning
    {
      CellBarGrid *panningDisplay = new CellBarGrid(31);
      if (module) {
        panningDisplay->cells = module->panningCells;
        panningDisplay->gridName = "Panning";
      }

      panningDisplay->box.pos = Vec(315, 104.5);
      panningDisplay->box.size = Vec(64, 64);
      addChild(panningDisplay);

      addInput(createInput<LightPort>(Vec(290, 100), module, FreudianSlipModule::PAN_SHIFT_X_CV));
      addInput(createInput<LightPort>(Vec(290, 127), module, FreudianSlipModule::PAN_SHIFT_Y_CV));
      addInput(createInput<LightPort>(Vec(290, 154), module, FreudianSlipModule::PAN_ROTATE_X_CV));

    }


      // Play Speed
    {
      CellBarGrid *playSpeedDisplay = new CellBarGrid(31);
      if (module) {
        playSpeedDisplay->cells = module->playSpeedCells;
        playSpeedDisplay->gridName = "Play Speed";
      } 

      playSpeedDisplay->box.pos = Vec(224, 104.5);
      playSpeedDisplay->box.size = Vec(64, 64);
      addChild(playSpeedDisplay);

      addInput(createInput<LightPort>(Vec(199, 100), module, FreudianSlipModule::PLAY_SPEED_SHIFT_X_CV));
      addInput(createInput<LightPort>(Vec(199, 127), module, FreudianSlipModule::PLAY_SPEED_SHIFT_Y_CV));
      addInput(createInput<LightPort>(Vec(199, 154), module, FreudianSlipModule::PLAY_SPEED_ROTATE_X_CV));

    }
        

    addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH-12, 0)));
    addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH + 12, 0)));
    addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH-12, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
    addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH + 12, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

  }


  void appendContextMenu(Menu* menu) override {
		FreudianSlipModule* fs = dynamic_cast<FreudianSlipModule*>(this->module);
		assert(fs);

		menu->addChild(new MenuLabel());

    PLAYERItem *rootDirItem = new PLAYERItem;
		rootDirItem->text = "Load Sample";
		rootDirItem->hsm = fs;
		menu->addChild(rootDirItem);

		{
      OptionsMenuItem* mi = new OptionsMenuItem("Window Function");
			mi->addItem(OptionMenuItem("None", [fs]() { return fs->windowFunctionId == 0; }, [fs]() { fs->windowFunctionId = 0; }));
			mi->addItem(OptionMenuItem("Triangle", [fs]() { return fs->windowFunctionId == 1; }, [fs]() { fs->windowFunctionId = 1; }));
			mi->addItem(OptionMenuItem("Welch", [fs]() { return fs->windowFunctionId == 2; }, [fs]() { fs->windowFunctionId = 2; }));
			mi->addItem(OptionMenuItem("Sine", [fs]() { return fs->windowFunctionId == 3; }, [fs]() { fs->windowFunctionId = 3; }));
			mi->addItem(OptionMenuItem("Hanning", [fs]() { return fs->windowFunctionId == 4; }, [fs]() { fs->windowFunctionId = 4; }));
			mi->addItem(OptionMenuItem("Blackman", [fs]() { return fs->windowFunctionId == 5; }, [fs]() { fs->windowFunctionId = 5; }));
			mi->addItem(OptionMenuItem("Nutall", [fs]() { return fs->windowFunctionId == 6; }, [fs]() { fs->windowFunctionId = 6; }));
			mi->addItem(OptionMenuItem("Kaiser", [fs]() { return fs->windowFunctionId == 7; }, [fs]() { fs->windowFunctionId = 7; }));
			OptionsMenuItem::addToMenu(mi, menu);
		}
  }
};

Model *modelFreudianSlip = createModel<FreudianSlipModule, FreudianSlipWidget>("FreudianSlip");
