#ifndef FIELD_H
#define FIELD_H

#include <iostream>
#include <memory>

#include "utils.h"
#include "random.h"
#include "constants.h"

using namespace std;

extern uint64_t base_hash;
extern uint64_t bot_hash[2][FIELD_SIZE];
extern uint64_t field_hash[FIELD_SIZE];

void InitHashConstants();

const int dd[4] = {
    1,       // right
    WIDTH,   // down
    -1,      // left
    -HEIGHT, // up
};

struct Field {
  // 0 - empty
  // 1 - full
  int8_t walls[WIDTH*HEIGHT] = {0};

  int16_t bots[2] = {0, 0};

  uint64_t hash = 0;

  // 0 - no winner
  // 1 - player 1 won
  // 2 - player 2 won
  // 3 - draw
  int8_t winner = 0;

  Field() {
  }

  inline bool MoveBots(const int dir[2]) {

    hash ^= bot_hash[0][bots[0]];
    hash ^= bot_hash[1][bots[1]];


    bots[0] += dir[0];
    bots[1] += dir[1];

    bool died[2];

    if (walls[bots[0]] == 1) {
      died[0] = true;
    }
    if (walls[bots[1]] == 1) {
      died[1] = true;
    }

    if (died[0] or died[1]) {
      if (died[0] and died[1]) {
        winner = 3;
      } else if (died[0]) {
        winner = 1;
      } else {
        winner = 2;
      }
    }

    walls[bots[0]] = 1;
    walls[bots[1]] = 1;

    hash ^= bot_hash[0][bots[0]];
    hash ^= bot_hash[1][bots[1]];

    hash ^= field_hash[bots[0]];
    hash ^= field_hash[bots[1]];

    return true;
  }

  inline bool Over() const {
    return winner != 0;
  }

  inline int Winner() const {
    return winner;
  }

  inline int ValidMoves(int player, int dirs[4]) const {
    int n = 0;
    for (int d = 0; d < 4; d++) {
      int p = bots[player] + dd[d];
      if (walls[p] == 1) continue;
      dirs[n++] = d;
    }
    return n;
  }

  uint64_t ComputeHash() const;
};

void PrintField(const Field& field);

unique_ptr<Field> ParseField(const string& repr);

#endif
