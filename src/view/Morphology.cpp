#include "../controller/Morphology.hpp"

#include "../component/knob.hpp"
#include "../component/light.hpp"
#include "../component/port.hpp"
#include "../component/button.hpp"
#include "../component/display.hpp"
#include "../component/menu.hpp"
//#include "../component/menu.cpp"

struct MorphologyWidget : ModuleWidget {
  MorphologyWidget(MorphologyModule *module);

  void appendContextMenu(Menu* menu) override {
		MorphologyModule* m = dynamic_cast<MorphologyModule*>(this->module);
		assert(m);

		menu->addChild(new MenuLabel());
		{
      OptionsMenuItem* mi = new OptionsMenuItem("# of Frequency Bands");
			mi->addItem(OptionMenuItem("1024", [m]() { return m->frameSize == 11; }, [m]() { m->frameSize = 11; }));
			mi->addItem(OptionMenuItem("2048", [m]() { return m->frameSize == 12; }, [m]() { m->frameSize = 12; }));
			mi->addItem(OptionMenuItem("4096", [m]() { return m->frameSize == 13; }, [m]() { m->frameSize = 13; }));
			OptionsMenuItem::addToMenu(mi, menu);
		}
		{
      OptionsMenuItem* mi = new OptionsMenuItem("Window Function");
			mi->addItem(OptionMenuItem("None", [m]() { return m->windowFunctionId == 0; }, [m]() { m->windowFunctionId = 0; }));
			mi->addItem(OptionMenuItem("Triangle", [m]() { return m->windowFunctionId == 1; }, [m]() { m->windowFunctionId = 1; }));
			mi->addItem(OptionMenuItem("Welch", [m]() { return m->windowFunctionId == 2; }, [m]() { m->windowFunctionId = 2; }));
			mi->addItem(OptionMenuItem("Sine", [m]() { return m->windowFunctionId == 3; }, [m]() { m->windowFunctionId = 3; }));
			mi->addItem(OptionMenuItem("Hanning", [m]() { return m->windowFunctionId == 4; }, [m]() { m->windowFunctionId = 4; }));
			mi->addItem(OptionMenuItem("Blackman", [m]() { return m->windowFunctionId == 5; }, [m]() { m->windowFunctionId = 5; }));
			mi->addItem(OptionMenuItem("Nutall", [m]() { return m->windowFunctionId == 6; }, [m]() { m->windowFunctionId = 6; }));
			mi->addItem(OptionMenuItem("Kaiser", [m]() { return m->windowFunctionId == 7; }, [m]() { m->windowFunctionId = 7; }));
			OptionsMenuItem::addToMenu(mi, menu);
		}
  }
};

MorphologyWidget::MorphologyWidget(MorphologyModule *module) {
  setModule(module);
  box.size = Vec(20 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT);

  setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Morphology.svg")));

  {
    CellGrid *bandShiftDisplay = new CellGrid();
    if (module) {
      bandShiftDisplay->cells = module->bandShiftCells;
    }

    bandShiftDisplay->box.pos = Vec(40, 40);
    bandShiftDisplay->box.size = Vec(100, 256);
    addChild(bandShiftDisplay);

    addInput(createInput<LightPort>(Vec(80, 300), module, MorphologyModule::BAND_SHIFT_X_CV));
    addInput(createInput<LightPort>(Vec(15, 150), module, MorphologyModule::BAND_SHIFT_Y_CV));

  }

  {
    CellGrid *panningDisplay = new CellGrid();
    if (module) {
      panningDisplay->cells = module->panningCells;
    }

    panningDisplay->box.pos = Vec(180, 40);
    panningDisplay->box.size = Vec(100, 256);
    addChild(panningDisplay);

    addInput(createInput<LightPort>(Vec(220, 300), module, MorphologyModule::PANNING_X_CV));
    addInput(createInput<LightPort>(Vec(155, 150), module, MorphologyModule::PANNING_Y_CV));

  }


  addParam(createParam<RecButton>(Vec(8, 316), module, MorphologyModule::INVERT_SPECTA_1));
  addChild(createLight<LargeSMLight<RectangleLight<RedGreenBlueLight>>>(Vec(10, 317), module, MorphologyModule::INVERT_SPECTA_1_LIGHT));

  addParam(createParam<RecButton>(Vec(42, 316), module, MorphologyModule::INVERT_SPECTA_2));
  addChild(createLight<LargeSMLight<RectangleLight<RedGreenBlueLight>>>(Vec(44, 317), module, MorphologyModule::INVERT_SPECTA_2_LIGHT));


  addInput(createInput<LightPort>(Vec(84, 348), module, MorphologyModule::SPREAD_CV));
  addParam(createParam<LightKnob>(Vec(113.25, 324), module, MorphologyModule::SPREAD));
  {
    ArcDisplay *c = new ArcDisplay();
    if (module) {
      c->percentage = &module->bandShiftSpreadPercentage;
    } 
    c->box.pos = Vec(120, 330.5);
    c->box.size = Vec(60, 60);
    addChild(c);
  }

  addInput(createInput<LightPort>(Vec(179, 340), module, MorphologyModule::INPUT_1));
  addInput(createInput<LightPort>(Vec(201, 340), module, MorphologyModule::INPUT_2));

  addOutput(createOutput<LightPort>(Vec(242, 340), module, MorphologyModule::OUTPUT_L));
  addOutput(createOutput<LightPort>(Vec(264, 340), module, MorphologyModule::OUTPUT_R));

  addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH-12, 0)));
  addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH + 12, 0)));
  addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH-12, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
  addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH + 12, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

}

Model *modelMorphology = createModel<MorphologyModule, MorphologyWidget>("Morphology");
