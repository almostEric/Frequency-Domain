#include "../controller/GrainsOfWrath.hpp"

#include "../component/knob.hpp"
#include "../component/light.hpp"
#include "../component/port.hpp"
#include "../component/button.hpp"
#include "../component/display.hpp"
#include "../component/menu.hpp"


struct GWClearPLAYERItem : MenuItem {
	GrainsOfWrathModule *hsm ;
	void onAction(const event::Action &e) override {
		hsm->clearSamples();					
	}
};

struct GWDirPLAYERItem : MenuItem {
	GrainsOfWrathModule *hsm ;
	void onAction(const event::Action &e) override {
		
		char *path = osdialog_file(OSDIALOG_OPEN, NULL, NULL, NULL);        //////////dir.c_str(),
		if (path) {
			hsm->loadDirectory(path);
			free(path);
		}
	}
};


struct GWPLAYERItem : MenuItem {
	GrainsOfWrathModule *hsm ;
  int slot = 0;
	void onAction(const event::Action &e) override {
		
		char *path = osdialog_file(OSDIALOG_OPEN, NULL, NULL, NULL);        //////////dir.c_str(),
		if (path) {
			hsm->loadSample(slot,path);
			hsm->samplePosition[slot] = 0;
			hsm->lastPath[slot] = std::string(path);
			free(path);
		}
	}

  void step() override {
			rightText = (hsm->totalSampleC[slot] > 0) ? "✔" : "";
		}
};

struct GWDisplayHistogram : FramebufferWidget {
  GrainsOfWrathModule *module;

  GWDisplayHistogram() {
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

    nvgStrokeColor(args.vg, nvgRGB(0xb0,0xb0,0xff)); 	

    for(uint64_t i=0;i<module->grains.size();i++) {
        float y = module->individualGrainVoice[i];
        float x = float(module->sampleCounter - module->individualGrainSpawnTime[i]) / module->sampleRate * 500; 
        float pitch = module->individualGrainPitch[i];
        float grainSize = module->grains[i].size() / module->sampleRate * 75.0;

        nvgFillColor(args.vg, nvgRGB((pitch < 1 ? 128 + 16 / pitch : 0), (pitch > 1 ? 128+ pitch * 16 : 0) , 164.0 * (1.0 - std::min(std::abs(1.0-pitch),1.0)))); 	
        nvgBeginPath(args.vg);
        nvgRect(args.vg,x,y*3.125,grainSize,3.125);
        nvgFill(args.vg);
        if(module->individualGrainReversed[i]) {
          nvgStrokeWidth(args.vg,0.5);
          nvgStroke(args.vg);
        }
    }
    

   
  }
};



struct GWDisplaySampleFileInfo : FramebufferWidget {
  std::shared_ptr<Font> font;
  GrainsOfWrathModule *module;

  GWDisplaySampleFileInfo() {

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

    nvgFontSize(args.vg, 6);
    nvgFontFaceId(args.vg, font->handle);
    nvgTextLetterSpacing(args.vg, -1);
    nvgFillColor(args.vg, nvgRGB(0x3a, 0xa3, 0x27)); //CRT Green	
    for(uint8_t i=0;i<MAX_SAMPLES;i++) {
      nvgTextBox(args.vg, 2,5+ i*6,120, module->sampleStatusDesc[i].c_str(), NULL);
    }
  }
};


struct GrainsOfWrathWidget : ModuleWidget {
  
  GrainsOfWrathWidget(GrainsOfWrathModule *module) {
    setModule(module);
    box.size = Vec(30 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT);

    setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/GrainsOfWrath.svg")));

    {
      GWDisplaySampleFileInfo *dsf = new GWDisplaySampleFileInfo();
      //if (module) {
        dsf->module = module;
      //}
      dsf->box.pos = Vec(5.5, 23.5);
      dsf->box.size = Vec(127, 100);
      addChild(dsf);
    }

    {
      GWDisplayHistogram *dh = new GWDisplayHistogram();
      //if (module) {
        dh->module = module;
      //}
      dh->box.pos = Vec(141, 23.5);
      dh->box.size = Vec(328, 100);
      addChild(dh);
    }


    

    addParam(createParam<LightSmallKnob>(Vec(247.5, 215), module, GrainsOfWrathModule::START_POS_PARAM));
    {
      SmallArcDisplay *c = new SmallArcDisplay();
      if (module) {
        c->percentage = &module->startPositionPercentage;
      }
      c->box.pos = Vec(251, 218.5);
      c->box.size = Vec(30, 30);
      addChild(c);
    }

    addParam(createParam<LightSmallKnob>(Vec(337.5, 215), module, GrainsOfWrathModule::STOP_POS_PARAM));
    {
      SmallArcDisplay *c = new SmallArcDisplay();
      if (module) {
        c->percentage = &module->stopPositionPercentage;
      }
      c->box.pos = Vec(341, 218.5);
      c->box.size = Vec(30, 30);
      addChild(c);
    }

    addParam(createParam<LightSmallKnob>(Vec(428.5, 215), module, GrainsOfWrathModule::PLAY_SPEED_PARAM));
    {
      SmallBidirectionalArcDisplay *c = new SmallBidirectionalArcDisplay();
      if (module) {
        c->percentage = &module->playSpeedPercentage;
      }
      c->box.pos = Vec(432, 218.5);
      c->box.size = Vec(30, 30);
      addChild(c);
    }


    addParam(createParam<LightSmallKnobSnap>(Vec(46.5, 327), module, GrainsOfWrathModule::WINDOW_FUNCTION_PARAM));
    {
      SmallArcDisplay *c = new SmallArcDisplay();
      if (module) {
        c->percentage = &module->windowFunctionPercentage;
      }
      c->box.pos = Vec(50, 330.5);
      c->box.size = Vec(30, 30);
      addChild(c);
    }


    addParam(createParam<LightSmallKnob>(Vec(96.5, 198), module, GrainsOfWrathModule::GRAIN_DENSITY_PARAM));
    {
      SmallArcDisplay *c = new SmallArcDisplay();
      if (module) {
        c->percentage = &module->grainDensityPercentage;
      }
      c->box.pos = Vec(100, 201.5);
      c->box.size = Vec(30, 30);
      addChild(c);
    }
    addInput(createInput<LightPort>(Vec(118, 197), module, GrainsOfWrathModule::GRAIN_DENSITY_INPUT));


    addParam(createParam<LightSmallKnob>(Vec(151.5, 198), module, GrainsOfWrathModule::GRAIN_DENSITY_VARIATION_PARAM));
    {
      SmallArcDisplay *c = new SmallArcDisplay();
      if (module) {
        c->percentage = &module->grainDensityVariationPercentage;
      }
      c->box.pos = Vec(155, 201.5);
      c->box.size = Vec(30, 30);
      addChild(c);
    }
    addInput(createInput<LightPort>(Vec(173, 197), module, GrainsOfWrathModule::GRAIN_DENSITY_VARIATION_INPUT));


    addParam(createParam<LightSmallKnob>(Vec(157.5, 327), module, GrainsOfWrathModule::GRAIN_LENGTH_PARAM));
    {
      SmallArcDisplay *c = new SmallArcDisplay();
      if (module) {
        c->percentage = &module->grainLengthPercentage;
      }
      c->box.pos = Vec(161, 330.5);
      c->box.size = Vec(30, 30);
      addChild(c);
    }

    addParam(createParam<LightSmallKnob>(Vec(247.5, 327), module, GrainsOfWrathModule::GRAIN_PITCH_PARAM));
    {
      SmallBidirectionalArcDisplay *c = new SmallBidirectionalArcDisplay();
      if (module) {
        c->percentage = &module->grainPitchPercentage;
      }
      c->box.pos = Vec(251, 330.5);
      c->box.size = Vec(30, 30);
      addChild(c);
    }


    addParam(createParam<LightSmallKnob>(Vec(100, 155), module, GrainsOfWrathModule::VOICE_WEIGHT_SCALING_PARAM));
    {
      SmallArcDisplay *c = new SmallArcDisplay();
      if (module) {
        c->percentage = &module->weightScalingPercentage;
      }
      c->box.pos = Vec(103.5, 158.5);
      c->box.size = Vec(30, 30);
      addChild(c);
    }

    addParam(createParam<RecButton>(Vec(315, 255), module, GrainsOfWrathModule::PITCH_RANDOM_GAUSSIAN_MODE_PARAM));
    addChild(createLight<LargeSMLight<RectangleLight<RedGreenBlueLight>>>(Vec(317, 256), module, GrainsOfWrathModule::PITCH_GAUSSIAN_LIGHT));

    addParam(createParam<RecButton>(Vec(315, 290), module, GrainsOfWrathModule::PAN_RANDOM_GAUSSIAN_MODE_PARAM));
    addChild(createLight<LargeSMLight<RectangleLight<RedGreenBlueLight>>>(Vec(317, 291), module, GrainsOfWrathModule::PAN_GAUSSIAN_LIGHT));


    // // Input/Output
    addInput(createInput<LightPort>(Vec(105, 342), module, GrainsOfWrathModule::LIVE_INPUT));

    addInput(createInput<LightPort>(Vec(153.5, 153), module, GrainsOfWrathModule::EXTERNAL_CLOCK_INPUT));


    addInput(createInput<LightPort>(Vec(178, 342), module, GrainsOfWrathModule::LIVE_FREEZE_INPUT));

    addParam(createParam<TL1105>(Vec(250, 357), module, GrainsOfWrathModule::FREEZE_TRIGGER_MODE_PARAM));
		addChild(createLight<SmallLight<RedGreenBlueLight>>(Vec(240, 367), module, GrainsOfWrathModule::FREEZE_TRIGGER_MODE_TRIGGER_LIGHT));
		addChild(createLight<SmallLight<RedGreenBlueLight>>(Vec(240, 357), module, GrainsOfWrathModule::FREEZE_TRIGGER_MODE_GATE_LIGHT));

    addInput(createInput<LightPort>(Vec(276, 342), module, GrainsOfWrathModule::REVERSE_GRAIN_SAMPLES_INPUT));
    addInput(createInput<LightPort>(Vec(298, 342), module, GrainsOfWrathModule::REVERSE_GRAIN_LIVE_INPUT));


    addInput(createInput<LightPort>(Vec(336, 342), module, GrainsOfWrathModule::V_OCT_SAMPLE_INPUT));
    addInput(createInput<LightPort>(Vec(358, 342), module, GrainsOfWrathModule::V_OCT_LIVE_INPUT));


    addOutput(createOutput<LightPort>(Vec(411, 342), module, GrainsOfWrathModule::OUTPUT_L));
    addOutput(createOutput<LightPort>(Vec(433, 342), module, GrainsOfWrathModule::OUTPUT_R));



//CELLS
    // Voice Weighting
    {
      CellGrid *voiceWeightingDisplay = new CellGrid();
      if (module) {
        voiceWeightingDisplay->cells = module->voiceWeightCells;
        voiceWeightingDisplay->gridName = "Voice Weighting";
      }

      voiceWeightingDisplay->box.pos = Vec(24, 144.5);
      voiceWeightingDisplay->box.size = Vec(64, 64);
      addChild(voiceWeightingDisplay);

      addInput(createInput<LightPort>(Vec(0, 140), module, GrainsOfWrathModule::VOICE_WEIGHT_SHIFT_X_CV));
      addInput(createInput<LightPort>(Vec(0, 167), module, GrainsOfWrathModule::VOICE_WEIGHT_SHIFT_Y_CV));
      addInput(createInput<LightPort>(Vec(0, 194), module, GrainsOfWrathModule::VOICE_WEIGHT_ROTATE_X_CV));
    }


    // Start
    {
      CellBarGrid *startPosDisplay = new CellBarGrid();
      if (module) {
        startPosDisplay->cellWidth = 1.0; // Half width / double precision
        startPosDisplay->cellHeight = 4.0; // Double height
        startPosDisplay->cells = module->startPositionCells;
        startPosDisplay->gridName = "Start Position";
      }

      startPosDisplay->box.pos = Vec(224, 144.5);
      startPosDisplay->box.size = Vec(64, 64);
      addChild(startPosDisplay);

      addInput(createInput<LightPort>(Vec(199, 140), module, GrainsOfWrathModule::START_POS_SHIFT_X_CV));
      addInput(createInput<LightPort>(Vec(199, 167), module, GrainsOfWrathModule::START_POS_SHIFT_Y_CV));
      addInput(createInput<LightPort>(Vec(199, 194), module, GrainsOfWrathModule::START_POS_ROTATE_X_CV));

    }


    // Stop
    {
      CellBarGrid *stopPosDisplay = new CellBarGrid();
      if (module) {
        stopPosDisplay->cellWidth = 1.0; // Half width / double precision
        stopPosDisplay->cellHeight = 4.0; // Double height
        stopPosDisplay->cells = module->stopPositionCells;
        stopPosDisplay->gridName = "Stop Position";
      }

      stopPosDisplay->box.pos = Vec(314, 144.5);
      stopPosDisplay->box.size = Vec(64, 64);
      addChild(stopPosDisplay);

      addInput(createInput<LightPort>(Vec(290, 140), module, GrainsOfWrathModule::STOP_POS_SHIFT_X_CV));
      addInput(createInput<LightPort>(Vec(290, 167), module, GrainsOfWrathModule::STOP_POS_SHIFT_Y_CV));
      addInput(createInput<LightPort>(Vec(290, 194), module, GrainsOfWrathModule::STOP_POS_ROTATE_X_CV));

    }


    // Play Speed
    {
      CellBarGrid *playSpeedDisplay = new CellBarGrid(31);
      if (module) {
        playSpeedDisplay->cellWidth = 1.0; // Half width / double precision
        playSpeedDisplay->cellHeight = 4.0; // Double height
        playSpeedDisplay->cells = module->playSpeedCells;
        playSpeedDisplay->gridName = "Play Speed";
      } 

      playSpeedDisplay->box.pos = Vec(405, 144.5);
      playSpeedDisplay->box.size = Vec(64, 64);
      addChild(playSpeedDisplay);

      addInput(createInput<LightPort>(Vec(380, 140), module, GrainsOfWrathModule::PLAY_SPEED_SHIFT_X_CV));
      addInput(createInput<LightPort>(Vec(380, 167), module, GrainsOfWrathModule::PLAY_SPEED_SHIFT_Y_CV));
      addInput(createInput<LightPort>(Vec(380, 194), module, GrainsOfWrathModule::PLAY_SPEED_ROTATE_X_CV));
    }

    // Window Function
    {
      CellBarGrid *windowFunctionDisplay = new CellBarGrid();
      if (module) {
        windowFunctionDisplay->cells = module->windowFunctionCells;
        windowFunctionDisplay->gridName = "Window Function";
      }

      windowFunctionDisplay->box.pos = Vec(24, 253);
      windowFunctionDisplay->box.size = Vec(64, 64);
      addChild(windowFunctionDisplay);

      addInput(createInput<LightPort>(Vec(0, 250), module, GrainsOfWrathModule::WINDOW_FUNCTION_SHIFT_X_CV));
      addInput(createInput<LightPort>(Vec(0, 277), module, GrainsOfWrathModule::WINDOW_FUNCTION_SHIFT_Y_CV));
      addInput(createInput<LightPort>(Vec(0, 304), module, GrainsOfWrathModule::WINDOW_FUNCTION_ROTATE_X_CV));
    }

    // Grain Length
    {
      CellBarGrid *grainLengthDisplay = new CellBarGrid();
      if (module) {
        grainLengthDisplay->cellWidth = 1.0; // Half width / double precision
        grainLengthDisplay->cells = module->grainLengthCells;
        grainLengthDisplay->gridName = "Grain Length";
      }

      grainLengthDisplay->box.pos = Vec(134, 253);
      grainLengthDisplay->box.size = Vec(64, 64);
      addChild(grainLengthDisplay);

      addInput(createInput<LightPort>(Vec(109, 250), module, GrainsOfWrathModule::GRAIN_LENGTH_SHIFT_X_CV));
      addInput(createInput<LightPort>(Vec(109, 277), module, GrainsOfWrathModule::GRAIN_LENGTH_SHIFT_Y_CV));
      addInput(createInput<LightPort>(Vec(109, 304), module, GrainsOfWrathModule::GRAIN_LENGTH_ROTATE_X_CV));
    }

    // Grain Pitch
    {
      CellRangeGrid *grainPitchDisplay = new CellRangeGrid();
      if (module) {      
        grainPitchDisplay->cellWidth = 1.0; // Half width / double precision
        grainPitchDisplay->cells = module->grainPitchCells;
        grainPitchDisplay->gridName = "Grain Pitch";
      }

      grainPitchDisplay->box.pos = Vec(225, 253);
      grainPitchDisplay->box.size = Vec(64, 64);
      addChild(grainPitchDisplay);

      addInput(createInput<LightPort>(Vec(200, 250), module, GrainsOfWrathModule::GRAIN_PITCH_SHIFT_X_CV));
      addInput(createInput<LightPort>(Vec(200, 277), module, GrainsOfWrathModule::GRAIN_PITCH_SHIFT_Y_CV));
      addInput(createInput<LightPort>(Vec(200, 304), module, GrainsOfWrathModule::GRAIN_PITCH_ROTATE_X_CV));

      addInput(createInput<LightPort>(Vec(290, 250), module, GrainsOfWrathModule::GRAIN_PITCH_RANDOM_SHIFT_X_CV));
      addInput(createInput<LightPort>(Vec(290, 277), module, GrainsOfWrathModule::GRAIN_PITCH_RANDOM_SHIFT_Y_CV));
      addInput(createInput<LightPort>(Vec(290, 304), module, GrainsOfWrathModule::GRAIN_PITCH_RANDOM_ROTATE_X_CV));

    }


    // Panning
    {
      CellRangeGrid *panningDisplay = new CellRangeGrid();
      if (module) {
        panningDisplay->cells = module->panningCells;
        panningDisplay->gridName = "Panning";
      }

      panningDisplay->box.pos = Vec(369, 253);
      panningDisplay->box.size = Vec(64, 64);
      addChild(panningDisplay);

      addInput(createInput<LightPort>(Vec(344, 250), module, GrainsOfWrathModule::PAN_SHIFT_X_CV));
      addInput(createInput<LightPort>(Vec(344, 277), module, GrainsOfWrathModule::PAN_SHIFT_Y_CV));
      addInput(createInput<LightPort>(Vec(344, 304), module, GrainsOfWrathModule::PAN_ROTATE_X_CV));

      addInput(createInput<LightPort>(Vec(434, 250), module, GrainsOfWrathModule::PAN_RANDOM_SHIFT_X_CV));
      addInput(createInput<LightPort>(Vec(434, 277), module, GrainsOfWrathModule::PAN_RANDOM_SHIFT_Y_CV));
      addInput(createInput<LightPort>(Vec(434, 304), module, GrainsOfWrathModule::PAN_RANDOM_ROTATE_X_CV));

    }



      
    addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH-12, 0)));
    addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH + 12, 0)));
    addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH-12, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
    addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH + 12, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

  }


  void appendContextMenu(Menu* menu) override {
		GrainsOfWrathModule* gw = dynamic_cast<GrainsOfWrathModule*>(this->module);
		assert(gw);

		menu->addChild(new MenuLabel());

    
    GWClearPLAYERItem *clearSamples = new GWClearPLAYERItem;
    clearSamples->text = "Clear Samples";
    clearSamples->hsm = gw;
    menu->addChild(clearSamples);


    //Load Directory
    GWDirPLAYERItem *loadDirectory = new GWDirPLAYERItem;
    loadDirectory->text = "Load Directory";
    loadDirectory->hsm = gw;
    menu->addChild(loadDirectory);


    for(int slot=0;slot<MAX_SAMPLES;slot++) {
      GWPLAYERItem *sampleSlot = new GWPLAYERItem;
      sampleSlot->text = "Load Sample " + std::to_string(slot+1);
      sampleSlot->slot = slot;
      sampleSlot->hsm = gw;
      menu->addChild(sampleSlot);
    }

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

Model *modelGrainsOfWrath = createModel<GrainsOfWrathModule, GrainsOfWrathWidget>("GrainsOfWrath");
