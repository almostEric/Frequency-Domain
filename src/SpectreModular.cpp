#include "SpectreModular.hpp"

Plugin *pluginInstance;


void init(rack::Plugin *p) {
  pluginInstance = p;

  p->addModel(modelHarmonicConvergence);
  p->addModel(modelDelayedReaction);
  p->addModel(modelMorphology);
}
