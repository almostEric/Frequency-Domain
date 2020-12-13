#include "../controller/HarmonicConvergence.hpp"

#include "../component/knob.hpp"
#include "../component/light.hpp"
#include "../component/port.hpp"
#include "../component/button.hpp"
#include "../component/display.hpp"
//#include "../component/menu.hpp"
#include "../component/menu.cpp"

struct HarmonicConvergenceWidget : ModuleWidget {
  HarmonicConvergenceWidget(HarmonicConvergenceModule *module);

  void appendContextMenu(Menu* menu) override {
		HarmonicConvergenceModule* hc = dynamic_cast<HarmonicConvergenceModule*>(this->module);
		assert(hc);

		menu->addChild(new MenuLabel());
		{
      OptionsMenuItem* mi = new OptionsMenuItem("# of Frequency Bands for In 1");
			mi->addItem(OptionMenuItem("64", [hc]() { return hc->frameSize1 == 7; }, [hc]() { hc->frameSize1 = 7; }));
			mi->addItem(OptionMenuItem("128", [hc]() { return hc->frameSize1 == 8; }, [hc]() { hc->frameSize1 = 8; }));
			mi->addItem(OptionMenuItem("256", [hc]() { return hc->frameSize1 == 9; }, [hc]() { hc->frameSize1 = 9; }));
			mi->addItem(OptionMenuItem("512", [hc]() { return hc->frameSize1 == 10; }, [hc]() { hc->frameSize1 = 10; }));
			mi->addItem(OptionMenuItem("1024", [hc]() { return hc->frameSize1 == 11; }, [hc]() { hc->frameSize1 = 11; }));
			mi->addItem(OptionMenuItem("2048", [hc]() { return hc->frameSize1 == 12; }, [hc]() { hc->frameSize1 = 12; }));
			mi->addItem(OptionMenuItem("4096", [hc]() { return hc->frameSize1 == 13; }, [hc]() { hc->frameSize1 = 13; }));
			OptionsMenuItem::addToMenu(mi, menu);
		}
		{
      OptionsMenuItem* mi = new OptionsMenuItem("# of Frequency Bands for In 2");
			mi->addItem(OptionMenuItem("64", [hc]() { return hc->frameSize2 == 7; }, [hc]() { hc->frameSize2 = 7; }));
			mi->addItem(OptionMenuItem("128", [hc]() { return hc->frameSize2 == 8; }, [hc]() { hc->frameSize2 = 8; }));
			mi->addItem(OptionMenuItem("256", [hc]() { return hc->frameSize2 == 9; }, [hc]() { hc->frameSize2 = 9; }));
			mi->addItem(OptionMenuItem("512", [hc]() { return hc->frameSize2 == 10; }, [hc]() { hc->frameSize2 = 10; }));
			mi->addItem(OptionMenuItem("1024", [hc]() { return hc->frameSize2 == 11; }, [hc]() { hc->frameSize2 = 11; }));
			mi->addItem(OptionMenuItem("2048", [hc]() { return hc->frameSize2 == 12; }, [hc]() { hc->frameSize2 = 12; }));
			mi->addItem(OptionMenuItem("4096", [hc]() { return hc->frameSize2 == 13; }, [hc]() { hc->frameSize2 = 13; }));
			OptionsMenuItem::addToMenu(mi, menu);
		}
		{
      OptionsMenuItem* mi = new OptionsMenuItem("Window Function");
			mi->addItem(OptionMenuItem("None", [hc]() { return hc->windowFunctionId == 0; }, [hc]() { hc->windowFunctionId = 0; }));
			mi->addItem(OptionMenuItem("Triangle", [hc]() { return hc->windowFunctionId == 1; }, [hc]() { hc->windowFunctionId = 1; }));
			mi->addItem(OptionMenuItem("Welch", [hc]() { return hc->windowFunctionId == 2; }, [hc]() { hc->windowFunctionId = 2; }));
			mi->addItem(OptionMenuItem("Sine", [hc]() { return hc->windowFunctionId == 3; }, [hc]() { hc->windowFunctionId = 3; }));
			mi->addItem(OptionMenuItem("Hanning", [hc]() { return hc->windowFunctionId == 4; }, [hc]() { hc->windowFunctionId = 4; }));
			mi->addItem(OptionMenuItem("Blackman", [hc]() { return hc->windowFunctionId == 5; }, [hc]() { hc->windowFunctionId = 5; }));
			mi->addItem(OptionMenuItem("Nutall", [hc]() { return hc->windowFunctionId == 6; }, [hc]() { hc->windowFunctionId = 6; }));
			mi->addItem(OptionMenuItem("Kaiser", [hc]() { return hc->windowFunctionId == 7; }, [hc]() { hc->windowFunctionId = 7; }));
			OptionsMenuItem::addToMenu(mi, menu);
		}
  }
};

HarmonicConvergenceWidget::HarmonicConvergenceWidget(HarmonicConvergenceModule *module) {
  setModule(module);
  box.size = Vec(30 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT);

  setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/HarmonicConvergence.svg")));



  // VOICE_COUNT
  addInput(createInput<LightPort>(Vec(3, 51), module, HarmonicConvergenceModule::VOICE_COUNT_CV));
  addParam(createParam<LightKnobSnap>(Vec(33, 37), module, HarmonicConvergenceModule::VOICE_COUNT));

  {
    ArcDisplay *c = new ArcDisplay();
    if (module) {
      c->percentage = &module->voiceCountPercentage;
    }
    c->box.pos = Vec(39.25, 43.5);
    c->box.size = Vec(60, 60);
    addChild(c);
  }


  // Waveform
  addInput(createInput<LightPort>(Vec(66.5, 82), module, HarmonicConvergenceModule::VOICE_WAVEFORM_CV));
  addParam(createParam<LightKnobSnap>(Vec(99.25, 68), module, HarmonicConvergenceModule::VOICE_WAVEFORM));

  {
    ArcDisplay *c = new ArcDisplay();
    if (module) {
      c->percentage = &module->waveformPercentage;
    }
    c->box.pos = Vec(105.5, 74.5);
    c->box.size = Vec(60, 60);
    addChild(c);
  }


  // Frequeny Shift
  addInput(createInput<LightPort>(Vec(159.5, 36), module, HarmonicConvergenceModule::OCTAVE_CV));
  addParam(createParam<LightSmallKnob>(Vec(187.25, 40), module, HarmonicConvergenceModule::OCTAVE));

  {
    SmallBidirectionalArcDisplay *c = new SmallBidirectionalArcDisplay();
    if (module) {
      c->percentage = &module->octavePercentage;
    }
    c->box.pos = Vec(190.5, 43.5);
    c->box.size = Vec(30, 30);
    addChild(c);
  }

  //Frequency Wapring
  addInput(createInput<LightPort>(Vec(159.5, 82), module, HarmonicConvergenceModule::FREQ_WARP_AMOUNT_CV));
  addParam(createParam<LightSmallKnob>(Vec(187.25, 85), module, HarmonicConvergenceModule::FREQ_WARP_AMOUNT));
  {
    SmallArcDisplay *c = new SmallArcDisplay();
    if (module) {
      c->percentage = &module->freqWarpAmount;
    }
    c->box.pos = Vec(190.5, 88.5);
    c->box.size = Vec(30, 30);
    addChild(c);
  }


  // Warp Center Frequency
  addInput(createInput<LightPort>(Vec(224, 82), module, HarmonicConvergenceModule::FREQ_WARP_CENTER_CV));
  addParam(createParam<LightSmallKnob>(Vec(252.5, 85), module, HarmonicConvergenceModule::FREQ_WARP_CENTER));
  {
    SmallArcDisplay *c = new SmallArcDisplay();
    if (module) {
      c->percentage = &module->freqWarpCenterPercentage;
    }
    c->box.pos = Vec(256, 88.5);
    c->box.size = Vec(30, 30);
    addChild(c);
  }
  
  addParam(createParam<RecButton>(Vec(247, 64), module, HarmonicConvergenceModule::FREQ_WARP_USE_FUNDAMENTAL));
  addChild(createLight<LargeSMLight<RectangleLight<RedGreenBlueLight>>>(Vec(249, 65), module, HarmonicConvergenceModule::FREQ_WARP_USE_FUNDAMENTAL_LIGHT));


  // Analysis Center Frequency
  addInput(createInput<LightPort>(Vec(309.5, 36), module, HarmonicConvergenceModule::ANALYZE_CENTER_CV));
  addParam(createParam<LightSmallKnob>(Vec(337.25, 40), module, HarmonicConvergenceModule::ANALYZE_CENTER));
  {
    SmallArcDisplay *c = new SmallArcDisplay();
    if (module) {
      c->percentage = &module->analyzeCenterPercentage;
    }
    c->box.pos = Vec(340.5, 43.5);
    c->box.size = Vec(30, 30);
    addChild(c);
  }

    // Analysis Band Width
  addInput(createInput<LightPort>(Vec(386, 36), module, HarmonicConvergenceModule::ANALYZE_BW_CV));
  addParam(createParam<LightSmallKnob>(Vec(414.25, 40), module, HarmonicConvergenceModule::ANALYZE_BW));
  {
    SmallArcDisplay *c = new SmallArcDisplay();
    if (module) {
      c->percentage = &module->analyzeBWPercentage;
    }
    c->box.pos = Vec(417.5, 43.5);
    c->box.size = Vec(30, 30);
    addChild(c);
  }



  // Spectral Mode
  addInput(createInput<LightPort>(Vec(309.5, 82), module, HarmonicConvergenceModule::SPECTRAL_MODE_CV));
  addParam(createParam<LightSmallKnobSnap>(Vec(337.25, 85), module, HarmonicConvergenceModule::SPECTRAL_MODE));
  {
    SmallArcDisplay *c = new SmallArcDisplay();
    if (module) {
      c->percentage = &module->spectralPercentage;
    }
    c->box.pos = Vec(340.5, 88.5);
    c->box.size = Vec(30, 30);
    addChild(c);
  }

  // Voice Shift
  addInput(createInput<LightPort>(Vec(386.5, 82), module, HarmonicConvergenceModule::VOICE_SHIFT_CV));
  addParam(createParam<LightSmallKnobSnap>(Vec(414.25, 85), module, HarmonicConvergenceModule::VOICE_SHIFT));

  {
    SmallBidirectionalArcDisplay *c = new SmallBidirectionalArcDisplay();
    if (module) {
      c->percentage = &module->shiftPercentage;
    }
    c->box.pos = Vec(417.5, 88.5);
    c->box.size = Vec(30, 30);
    addChild(c);
  }


  // Mix
  addInput(createInput<LightPort>(Vec(362.5, 285), module, HarmonicConvergenceModule::MIX_CV));
  addParam(createParam<LightKnob>(Vec(395.25, 272), module, HarmonicConvergenceModule::MIX));

  {
    ArcDisplay *c = new ArcDisplay();
    if (module) {
      c->percentage = &module->mixPercentage;
    }
    c->box.pos = Vec(400.5, 278.5);
    c->box.size = Vec(60, 60);
    addChild(c);
  }

  addInput(createInput<LightPort>(Vec(303, 285), module, HarmonicConvergenceModule::FEEDBACK_CV));
  addParam(createParam<LightSmallKnob>(Vec(328.5, 289), module, HarmonicConvergenceModule::FEEDBACK));
  {
    SmallArcDisplay *c = new SmallArcDisplay();
    if (module) {
      c->percentage = &module->feedbackPercentage;
    }
    c->box.pos = Vec(332, 292);
    c->box.size = Vec(30, 30);
    addChild(c);
  }


  addParam(createParam<LightSmallKnob>(Vec(160.25, 227), module, HarmonicConvergenceModule::FM_AMOUNT));
  {
    SmallArcDisplay *c = new SmallArcDisplay();
    if (module) {
      c->percentage = &module->fmAmountPercentage;
    }
    c->box.pos = Vec(163.5, 230);
    c->box.size = Vec(30, 30);
    addChild(c);
  }

  addParam(createParam<LightSmallKnob>(Vec(388.25, 227), module, HarmonicConvergenceModule::RM_MIX));
  {
    SmallArcDisplay *c = new SmallArcDisplay();
    if (module) {
      c->percentage = &module->rmMixPercentage;
    }
    c->box.pos = Vec(391.5, 230);
    c->box.size = Vec(30, 30);
    addChild(c);
  }

  addParam(createParam<LightSmallKnob>(Vec(140.25, 354), module, HarmonicConvergenceModule::MORPH_AMOUNT));
  {
    SmallBidirectionalArcDisplay *c = new SmallBidirectionalArcDisplay();
    if (module) {
      c->percentage = &module->morphPercentage;
    }
    c->box.pos = Vec(143.5, 357);
    c->box.size = Vec(30, 30);
    addChild(c);
  }


  //Ring Modulator Enableds 
  addParam(createParam<RecButton>(Vec(300.5, 228), module, HarmonicConvergenceModule::RING_MODULATION));
  addChild(createLight<LargeSMLight<RectangleLight<RedGreenBlueLight>>>(Vec(302.5, 229), module, HarmonicConvergenceModule::RING_MODULATION_ENABLED_LIGHT));


  // Input/Output
  addInput(createInput<LightPort>(Vec(325, 340), module, HarmonicConvergenceModule::INPUT_1));
  addInput(createInput<LightPort>(Vec(347, 340), module, HarmonicConvergenceModule::INPUT_2));

  addOutput(createOutput<LightPort>(Vec(395, 340), module, HarmonicConvergenceModule::OUTPUT_L));
  addOutput(createOutput<LightPort>(Vec(417, 340), module, HarmonicConvergenceModule::OUTPUT_R));


  // FM Input
  addInput(createInput<LightPort>(Vec(18, 222), module, HarmonicConvergenceModule::FM_INPUT_1));
  addInput(createInput<LightPort>(Vec(40, 222), module, HarmonicConvergenceModule::FM_INPUT_2));

  // FM Input
  addInput(createInput<LightPort>(Vec(240, 222), module, HarmonicConvergenceModule::AM_RM_INPUT_1));
  addInput(createInput<LightPort>(Vec(262, 222), module, HarmonicConvergenceModule::AM_RM_INPUT_2));


  // Frame Size
  // addInput(createInput<LightPort>(Vec(297.5, 275), module, HarmonicConvergenceModule::FRAME_SIZE_CV));
  // addParam(createParam<LightKnobSnap>(Vec(324.25, 262), module, HarmonicConvergenceModule::FRAME_SIZE));

  // {
  //   ArcDisplay *c = new ArcDisplay();
  //   if (module) {
  //     c->percentage = &module->framePercentage;
  //   }
  //   c->box.pos = Vec(331.5, 268.5);
  //   c->box.size = Vec(60, 60);
  //   addChild(c);
  // }

  // FM Matrix
  {
    CellGrid *frequencyModulationDisplay = new CellGrid();
    if (module) {
      frequencyModulationDisplay->cells = module->frequencyModulationCells;
      frequencyModulationDisplay->gridName = "FM Matrix";
    }

    frequencyModulationDisplay->box.pos = Vec(35, 140);
    frequencyModulationDisplay->box.size = Vec(64, 72);
    addChild(frequencyModulationDisplay);

    addInput(createInput<LightPort>(Vec(10, 142), module, HarmonicConvergenceModule::FM_SHIFT_X_CV));
    addInput(createInput<LightPort>(Vec(10, 177), module, HarmonicConvergenceModule::FM_SHIFT_Y_CV));
  }

  // FM Amount
  {
    CellBarGrid *FMAmountDisplay = new CellBarGrid();
    if (module) {
      FMAmountDisplay->cells = module->frequencyModulationAmountCells;
      FMAmountDisplay->gridName = "FM Amount";
    }

    FMAmountDisplay->box.pos = Vec(130, 140);
    FMAmountDisplay->box.size = Vec(80, 72);
    addChild(FMAmountDisplay);

    addInput(createInput<LightPort>(Vec(105, 142), module, HarmonicConvergenceModule::FM_AMOUNT_SHIFT_X_CV));
    addInput(createInput<LightPort>(Vec(105, 177), module, HarmonicConvergenceModule::FM_AMOUNT_SHIFT_Y_CV));
  }

  // RM Matrix
  {
    CellGrid *ringModulationDisplay = new CellGrid();
    if (module) {
      ringModulationDisplay->cells = module->ringModulationCells;
      ringModulationDisplay->gridName = "AM/RM Matrix";
    }

    ringModulationDisplay->box.pos = Vec(255, 140);
    ringModulationDisplay->box.size = Vec(72, 72);
    addChild(ringModulationDisplay);

    addInput(createInput<LightPort>(Vec(230, 142), module, HarmonicConvergenceModule::RM_SHIFT_X_CV));
    addInput(createInput<LightPort>(Vec(230, 177), module, HarmonicConvergenceModule::RM_SHIFT_Y_CV));
  }

  // RM Mix
  {
    CellBarGrid *RMMixDisplay = new CellBarGrid(0);
    
    if (module) {
      RMMixDisplay->cells = module->ringModulationMixCells;
      RMMixDisplay->gridName = "AM/RM Mix";
    }

    RMMixDisplay->box.pos = Vec(358, 140);
    RMMixDisplay->box.size = Vec(80, 72);
    addChild(RMMixDisplay);

    addInput(createInput<LightPort>(Vec(333, 142), module, HarmonicConvergenceModule::RM_MIX_SHIFT_X_CV));
    addInput(createInput<LightPort>(Vec(333, 177), module, HarmonicConvergenceModule::RM_MIX_SHIFT_Y_CV));
  }


  // Morphing
  {
    CellBarGrid *morphingDisplay = new CellBarGrid(39);
    if (module) {
      morphingDisplay->cells = module->morphingCells;
      morphingDisplay->gridName = "Morphing";
    }

    morphingDisplay->box.pos = Vec(110, 276.5);
    morphingDisplay->box.size = Vec(80, 72);
    addChild(morphingDisplay);

    addInput(createInput<LightPort>(Vec(85, 277), module, HarmonicConvergenceModule::MORPH_SHIFT_X_CV));
    addInput(createInput<LightPort>(Vec(85, 313), module, HarmonicConvergenceModule::MORPH_SHIFT_Y_CV));

  }

  //Morph Mode
  // addParam(createParam<RecButton>(Vec(55.5, 280), module, HarmonicConvergenceModule::MORPH_MODE));
  // addChild(createLight<LargeSMLight<RectangleLight<RedGreenBlueLight>>>(Vec(57.5, 281), module, HarmonicConvergenceModule::MORPH_MODE_LIGHT));




  // Panning
  {
    CellBarGrid *panningDisplay = new CellBarGrid(39);
    if (module) {
      panningDisplay->cells = module->panningCells;
      panningDisplay->gridName = "Attenuation";
    }

    panningDisplay->box.pos = Vec(220, 276.5);
    panningDisplay->box.size = Vec(80, 72);
    addChild(panningDisplay);

    addInput(createInput<LightPort>(Vec(195, 277), module, HarmonicConvergenceModule::PAN_SHIFT_X_CV));
    addInput(createInput<LightPort>(Vec(195, 313), module, HarmonicConvergenceModule::PAN_SHIFT_Y_CV));

  }

  addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH-12, 0)));
  addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH + 12, 0)));
  addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH-12, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
  addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH + 12, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

}

Model *modelHarmonicConvergence = createModel<HarmonicConvergenceModule, HarmonicConvergenceWidget>("HarmonicConvergence");
