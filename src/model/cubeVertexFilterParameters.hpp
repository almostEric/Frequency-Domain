struct individualFilterParameters {

    individualFilterParameters() {
    }

    float Fc;
    float Q;
    float gain;
};

struct cubeVertexFilterParameters {

  float makeupGain;
  individualFilterParameters filterParameters[7];


};

