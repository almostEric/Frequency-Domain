#include "FrequencyDomain.hpp"

Plugin *pluginInstance;


void init(rack::Plugin *p) {
  pluginInstance = p;

  p->addModel(modelBallOfConfusion);
  p->addModel(modelDelayedReaction);
  p->addModel(modelFreudianSlip);
  p->addModel(modelGrainsOfWrath);
  p->addModel(modelHarmonicConvergence);
  p->addModel(modelMorphology);
}
