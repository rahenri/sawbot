#ifndef FIELD_H
#define FIELD_H

#include <iostream>
#include <memory>

#include "constants.h"
#include "nn.h"
#include "random.h"
#include "utils.h"

using namespace std;

extern uint64_t base_hash;
extern uint64_t bot_hash[2][FIELD_SIZE];
extern uint64_t field_hash[FIELD_SIZE];

void InitHashConstants();

static const int RIGHT = 0;
static const int DOWN = 1;
static const int LEFT = 2;
static const int UP = 3;

static const int FIRST_PLAYER = 0;
static const int SECOND_PLAYER = 1;

const int dd[] = {
    1,       // right
    WIDTH,   // down
    -1,      // left
    -HEIGHT, // up
};

const int dd8[] = {
    1,         // right      7
    1+WIDTH,   // right down 6
    WIDTH,     // down       5
    WIDTH-1,   // down left  4
    -1,        // left       3
    -1-HEIGHT, // left up    2
    -HEIGHT,   // up         1
    -HEIGHT+1, // up right   0
};

struct Field {
  // 0 - empty
  // 1 - full
  int8_t walls[FIELD_SIZE] = {0};

  int16_t bots[2] = {0, 0};

  uint64_t hash = 0;

  int ply = 0;

  bool died[2] = {false};

  mutable uint16_t dists[2][FIELD_SIZE];
  mutable int16_t queue[FIELD_SIZE*10];

  Field() {
    // Put wall fence
    for (int i = 0; i < WIDTH; i++) {
      walls[i] = 1;
      walls[i+FIELD_SIZE-WIDTH] = 1;
    }
    for (int i = 0; i < HEIGHT; i++) {
      walls[i*WIDTH] = 1;
      walls[i*WIDTH+WIDTH-1] = 1;
    }
  }

  inline void MoveBot(int player, int dir) {

    hash ^= bot_hash[player][bots[player]];
    bots[player] += dd[dir];
    hash ^= bot_hash[player][bots[player]];

    if (walls[bots[player]]) {
      died[player] = true;
    }

    if (not walls[bots[player]]) {
      hash ^= field_hash[bots[player]];
    }
    walls[bots[player]]++;

    ply++;
  }

  inline void UnmoveBot(int player, int dir) {

    walls[bots[player]]--;
    if (not walls[bots[player]]) {
      hash ^= field_hash[bots[player]];
    }

    hash ^= bot_hash[player][bots[player]];
    bots[player] -= dd[dir];
    hash ^= bot_hash[player][bots[player]];

    died[player] = false;

    ply--;
  }

  inline bool Over() const {
    return died[FIRST_PLAYER] or died[SECOND_PLAYER];
  }

  // Returns the player ID who won, returns -1 if drawn.
  inline int Winner() const {
    if (bots[FIRST_PLAYER] == bots[SECOND_PLAYER]) {
      return -1;
    }
    if (!died[FIRST_PLAYER]) {
      return FIRST_PLAYER;
    }
    if (!died[SECOND_PLAYER]) {
      return SECOND_PLAYER;
    }
    return -1;
  }

  inline int ValidMoves(int player, int moves[4]) const {
    int n = 0;
    for (int i = 0; i < 4; i++) {
      int p = bots[player] + dd[i];
      if (walls[p]) {
        continue;
      }
      moves[n++] = i;
    }
    if (n == 0) {
      if (bots[player]%WIDTH < (WIDTH/2)) {
        moves[n++] = RIGHT;
      } else {
        moves[n++] = LEFT;
      }
    }
    return n;
  }

  inline bool isTunnel(int pos) const {
    uint32_t mask = 0;
    for (int d = 0; d < 4; d++) {
      int npos = pos + dd[d];
      mask <<= 1;
      if (!walls[npos]) {
        mask |= 1;
      }
    }
    return (mask == ((1<<0) | (1<<2))) or (mask == ((1<<1) | (1<<3)));
    return false;
  }

  void PopulateDists() const {
    for (int p = 0; p < 2; p++) {
      if (died[p]) continue;
      int end = 0;
      int start = 0;
      for (int i = 0; i < FIELD_SIZE; i++) dists[p][i]=0xffff;
      queue[end++] = bots[p];
      dists[p][bots[p]] = 0;
      while (end > start) {
        int pos = queue[start++];
        uint16_t dist = dists[p][pos]+1;
        if (pos != bots[p] && isTunnel(pos)) dist += 2;
        // if (pos != bots[p] && isTunnel(pos)) dist += 100;
        for (int i = 0; i < 4; i++) {
          int npos = pos + dd[i];
          if (walls[npos] or dists[p][npos]<=dist) continue;
          // cerr << dists[p][npos] << " " << dist << endl;
          dists[p][npos] = dist;
          queue[end++] = npos;
        }
      }
    }
  }

  void PrepFeatures() const {
    PopulateDists();
  }

  int EdgesFeature() const {
    int score = 0;
    for (int i = 0; i < FIELD_SIZE; i++) {
      if (walls[i]) continue;
      int neighbors = 0;
      for (int d = 0; d < 4; d++) {
        int i2 = i + dd[d];
        if (!walls[i2]) {
          neighbors++;
        }
      }
      int d1 = dists[FIRST_PLAYER][i];
      int d2 = dists[SECOND_PLAYER][i];
      if (d1 < d2) {
        score += neighbors;
      } else if (d2 < d1) {
        score -= neighbors;
      }
    }
    return score;
  }

  int AreaFeature() const {
    int score = 0;
    for (int i = 0; i < FIELD_SIZE; i++) {
      if (walls[i]) continue;
      int d1 = dists[FIRST_PLAYER][i];
      int d2 = dists[SECOND_PLAYER][i];
      if (d1 < d2) {
        score += 1;
      } else if (d2 < d1) {
        score -= 1;
      }
    }
    return score;
  }

  int Connected() const {
    for (int i = 0; i < FIELD_SIZE; i++) {
      if (walls[i]) continue;
      int d1 = dists[FIRST_PLAYER][i];
      int d2 = dists[SECOND_PLAYER][i];
      if (d1 < 0xffff && d2 < 0xffff) {
        return 1;
      }
    }
    return 0;
  }

  int Eval(int player) const {
    PrepFeatures();
    float features[4];
    features[0] = ply;
    features[1] = EdgesFeature();
    features[2] = AreaFeature();
    features[3] = Connected();
    int score = int(round(100000 * NNEval(features)));
    if (player == FIRST_PLAYER) {
      return score;
    } else {
      return -score;
    }
  }

  string gen_features() const;

  uint64_t ComputeHash() const;
};

void PrintField(const Field& field);

unique_ptr<Field> ParseField(const string& repr);

#endif
