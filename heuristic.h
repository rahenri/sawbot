#ifndef HEURISTIC_H
#define HEURISTIC_H

#include <map>

using namespace std;

inline int FindOrDie(const string& key, const map<string, int> &values) {
  auto it = values.find(key);
  if (it == values.end()) {
    cerr << "Key: '" << key << "' not given\n" << flush;
    ASSERT(false);
  }
  return it->second;
}


struct HeuristicWeights {
  int dir_weights[4] = {1};

  HeuristicWeights() {
    dir_weights[DOWN] = 10000;
    dir_weights[RIGHT] = 2;
    dir_weights[UP] = 1000;
    dir_weights[LEFT] = 1;
  }

  void from_map(const map<string, int>& values) {
    ASSERT(values.size() == 4);
    dir_weights[UP] = FindOrDie("up_weight", values);
    dir_weights[LEFT] = FindOrDie("left_weight", values);
    dir_weights[RIGHT] = FindOrDie("right_weight", values);
    dir_weights[DOWN] = FindOrDie("down_weight", values);
  }
};

#endif
