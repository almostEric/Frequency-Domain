struct individualFilterParameters {

    individualFilterParameters() {
    }

    float Fc;
    float gain;
    float drive;

    // for biquads
    float Q;

    // for comb filters
    float feedforwardAmount;
    float feedbackAmount;
    float feedforwardGain;
    float feedbackGain;

    // for modal filters
    float decay;
    float phase;
};

struct cubeVertexFilterParameters {

  float makeupGain;
  individualFilterParameters filterParameters[7];


};

