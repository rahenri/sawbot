#include <cmath>

#include "model.h"

float NNEval(const float* input) {
  float output0[10];
  for (int i = 0; i < 10; i++) {
    float out = bias0[i];
    for (int j = 0; j < 4; j++) {
      out += input[j] * weights0[j][i];
    }
    if (out < 0) out = 0;
    output0[i] = out;
  }

  float output1[10];
  for (int i = 0; i < 10; i++) {
    float out = bias1[i];
    for (int j = 0; j < 10; j++) {
      out += output0[j] * weights1[j][i];
    }
    if (out < 0) out = 0;
    output1[i] = out;
  }

  float output2[3];
  float sum = 0;
  for (int i = 0; i < 3; i++) {
    float out = bias2[i];
    for (int j = 0; j < 10; j++) {
      out += output1[j] * weights2[j][i];
    }
    out = expf(out);
    output2[i] = out;
    sum += out;
  }

  for (int i = 0; i < 3; i++) {
    output2[i] /= sum;
  }

  return output2[0] - output2[2];
}
