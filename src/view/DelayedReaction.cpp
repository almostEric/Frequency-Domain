#include "../controller/DelayedReaction.hpp"

#include "../component/knob.hpp"
#include "../component/light.hpp"
#include "../component/port.hpp"
#include "../component/button.hpp"
#include "../component/display.hpp"
#include "../component/menu.hpp"
//#include "../component/menu.cpp"

struct DelayedReactionWidget : ModuleWidget {
  DelayedReactionWidget(DelayedReactionModule *module);

  void appendContextMenu(Menu* menu) override {
		DelayedReactionModule* dr = dynamic_cast<DelayedReactionModule*>(this->module);
		assert(dr);

		menu->addChild(new MenuLabel());
		{
      OptionsMenuItem* mi = new OptionsMenuItem("# of Frequency Bands");
			mi->addItem(OptionMenuItem("1024", [dr]() { return dr->frameSize == 11; }, [dr]() { dr->frameSize = 11; }));
			mi->addItem(OptionMenuItem("2048", [dr]() { return dr->frameSize == 12; }, [dr]() { dr->frameSize = 12; }));
			mi->addItem(OptionMenuItem("4096", [dr]() { return dr->frameSize == 13; }, [dr]() { dr->frameSize = 13; }));
			OptionsMenuItem::addToMenu(mi, menu);
		}
		{
      OptionsMenuItem* mi = new OptionsMenuItem("Window Function");
			mi->addItem(OptionMenuItem("None", [dr]() { return dr->windowFunctionId == 0; }, [dr]() { dr->windowFunctionId = 0; }));
			mi->addItem(OptionMenuItem("Triangle", [dr]() { return dr->windowFunctionId == 1; }, [dr]() { dr->windowFunctionId = 1; }));
			mi->addItem(OptionMenuItem("Welch", [dr]() { return dr->windowFunctionId == 2; }, [dr]() { dr->windowFunctionId = 2; }));
			mi->addItem(OptionMenuItem("Sine", [dr]() { return dr->windowFunctionId == 3; }, [dr]() { dr->windowFunctionId = 3; }));
			mi->addItem(OptionMenuItem("Hanning", [dr]() { return dr->windowFunctionId == 4; }, [dr]() { dr->windowFunctionId = 4; }));
			mi->addItem(OptionMenuItem("Blackman", [dr]() { return dr->windowFunctionId == 5; }, [dr]() { dr->windowFunctionId = 5; }));
			mi->addItem(OptionMenuItem("Nutall", [dr]() { return dr->windowFunctionId == 6; }, [dr]() { dr->windowFunctionId = 6; }));
			mi->addItem(OptionMenuItem("Kaiser", [dr]() { return dr->windowFunctionId == 7; }, [dr]() { dr->windowFunctionId = 7; }));
			OptionsMenuItem::addToMenu(mi, menu);
		}
  }
};

DelayedReactionWidget::DelayedReactionWidget(DelayedReactionModule *module) {
  setModule(module);
  box.size = Vec(20 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT);

  setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/DelayedReaction.svg")));

  {
    CellBarGrid *attenuationDisplay = new CellBarGrid();
    if (module) {
      attenuationDisplay->cells = module->attenuationCells;
    }

    attenuationDisplay->box.pos = Vec(40, 40);
    attenuationDisplay->box.size = Vec(100, 256);
    addChild(attenuationDisplay);

    addInput(createInput<LightPort>(Vec(80, 300), module, DelayedReactionModule::ATTENUATION_X_CV));
    addInput(createInput<LightPort>(Vec(15, 150), module, DelayedReactionModule::ATTENUATION_Y_CV));

    addParam(createParam<RecButton>(Vec(5, 195), module, DelayedReactionModule::PIN_ATTENUATION_0S));
    addChild(createLight<LargeSMLight<RectangleLight<RedGreenBlueLight>>>(Vec(7, 196), module, DelayedReactionModule::PIN_ATTENUATION_0S_LIGHT));


    addParam(createParam<RecButton>(Vec(5, 240), module, DelayedReactionModule::LINK_ATTENUATION));
    addChild(createLight<LargeSMLight<RectangleLight<RedGreenBlueLight>>>(Vec(7, 241), module, DelayedReactionModule::LINK_ATTENUATION_LIGHT));

  }

  {
    CellBarGrid *displayTimeDisplay = new CellBarGrid();
    if (module) {
      displayTimeDisplay->cells = module->delayTimeCells;
    }

    displayTimeDisplay->box.pos = Vec(180, 40);
    displayTimeDisplay->box.size = Vec(100, 256);
    addChild(displayTimeDisplay);

    DelayRangeDisplay *delayRangeDisplay = new DelayRangeDisplay();
    if (module) {
      delayRangeDisplay->delayRangeTime = &module->delayRangeTime;
    }

    delayRangeDisplay->box.pos = Vec(205, 313); 
    delayRangeDisplay->box.size = Vec(50, 50);
    addChild(delayRangeDisplay);


    addParam(createParam<RecButton>(Vec(178.5, 318), module, DelayedReactionModule::DELAY_RANGE));
    addChild(createLight<LargeSMLight<RectangleLight<RedGreenBlueLight>>>(Vec(180.5, 319), module, DelayedReactionModule::DELAY_RANGE_LIGHT));

    addInput(createInput<LightPort>(Vec(220, 300), module, DelayedReactionModule::DELAY_TIME_X_CV));
    addInput(createInput<LightPort>(Vec(155, 150), module, DelayedReactionModule::DELAY_TIME_Y_CV));

    addParam(createParam<RecButton>(Vec(145, 195), module, DelayedReactionModule::PIN_DELAY_TIME_0S));
    addChild(createLight<LargeSMLight<RectangleLight<RedGreenBlueLight>>>(Vec(147, 196), module, DelayedReactionModule::PIN_DELAY_TIME_0S_LIGHT));


    addParam(createParam<RecButton>(Vec(145, 240), module, DelayedReactionModule::LINK_DELAY_TIME));
    addChild(createLight<LargeSMLight<RectangleLight<RedGreenBlueLight>>>(Vec(147, 241), module, DelayedReactionModule::LINK_DELAY_TIME_LIGHT));

  }

  {
    CellBarGrid *feedbackDisplay = new CellBarGrid();
    if (module) {
      feedbackDisplay->cells = module->feedbackCells;
    }

    feedbackDisplay->box.pos = Vec(320, 40);
    feedbackDisplay->box.size = Vec(100, 256);
    addChild(feedbackDisplay);

    addInput(createInput<LightPort>(Vec(360, 300), module, DelayedReactionModule::DELAY_FEEDBACK_X_CV));
    addInput(createInput<LightPort>(Vec(295, 150), module, DelayedReactionModule::DELAY_FEEDBACK_Y_CV));

    addParam(createParam<RecButton>(Vec(285, 195), module, DelayedReactionModule::PIN_FEEDBACK_0S));
    addChild(createLight<LargeSMLight<RectangleLight<RedGreenBlueLight>>>(Vec(287, 196), module, DelayedReactionModule::PIN_FEEDBACK_0S_LIGHT));

    addParam(createParam<RecButton>(Vec(285, 240), module, DelayedReactionModule::LINK_FEEDBACK));
    addChild(createLight<LargeSMLight<RectangleLight<RedGreenBlueLight>>>(Vec(287, 241), module, DelayedReactionModule::LINK_FEEDBACK_LIGHT));


  }


  addInput(createInput<LightPort>(Vec(279.5, 340), module, DelayedReactionModule::MIX_CV));
  addParam(createParam<LightKnob>(Vec(309.5, 324), module, DelayedReactionModule::MIX));
  {
    ArcDisplay *c = new ArcDisplay();
    if (module) {
      c->percentage = &module->mixPercentage;
    }
    c->box.pos = Vec(315.5, 330.5);
    c->box.size = Vec(60, 60);
    addChild(c);
  }


  addInput(createInput<LightPort>(Vec(230, 340), module, DelayedReactionModule::INPUT));

  addOutput(createOutput<LightPort>(Vec(374.5, 340), module, DelayedReactionModule::OUTPUT));

}

Model *modelDelayedReaction = createModel<DelayedReactionModule, DelayedReactionWidget>("DelayedReaction");