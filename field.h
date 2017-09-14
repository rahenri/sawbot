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

static const int RIGHT = 0;
static const int DOWN = 1;
static const int LEFT = 2;
static const int UP = 3;

const int dd[4] = {
    1,       // right
    WIDTH,   // down
    -1,      // left
    -HEIGHT, // up
};

struct Field {
  // 0 - empty
  // 1 - full
  int8_t walls[FIELD_SIZE] = {0};

  int16_t bots[2] = {0, 0};

  uint64_t hash = 0;

  int ply = 0;

  bool died[2] = {false};

  int8_t flood[FIELD_SIZE];
  int16_t queue[FIELD_SIZE];

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

  // inline void MoveBots(const int dir[2]) {

  //   hash ^= bot_hash[0][bots[0]];
  //   hash ^= bot_hash[1][bots[1]];

  //   bots[0] += dir[0];
  //   bots[1] += dir[1];

  //   if (walls[bots[0]]) {
  //     died[0] = true;
  //   }
  //   if (walls[bots[1]]) {
  //     died[1] = true;
  //   }

  //   walls[bots[0]]++;
  //   walls[bots[1]]++;

  //   hash ^= bot_hash[0][bots[0]];
  //   hash ^= bot_hash[1][bots[1]];

  //   hash ^= field_hash[bots[0]];
  //   hash ^= field_hash[bots[1]];
  // }

  inline void MoveBot(int player, int dir) {

    hash ^= bot_hash[player][bots[player]];

    bots[player] += dd[dir];

    if (walls[bots[player]]) {
      died[player] = true;
    }

    hash ^= bot_hash[player][bots[player]];

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

    died[player] = false;

    hash ^= bot_hash[player][bots[player]];

    ply--;
  }

  inline bool Over() const {
    return died[0] or died[1];
  }

  // Returns the player ID who won, returns -1 if drawn.
  inline int Winner() const {
    if (bots[0] == bots[1]) {
      return -1;
    }
    if (!died[0]) {
      return 0;
    }
    if (!died[1]) {
      return 1;
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
    return n;
  }

  int Eval(int player) {
    int end = 0;
    int start = 0;
    int tally[2] = {0,0};
    for (int i = 0; i < FIELD_SIZE; i++) flood[i]=0;
    queue[end++] = bots[0];
    queue[end++] = bots[1];
    flood[bots[0]] = 0|4;
    flood[bots[1]] = 1|4;
    while (end > start) {
      int p = queue[start++];
      int o = flood[p]&1;
      for (int i = 0; i < 4; i++) {
        int np = p + dd[i];
        if (walls[np] or flood[np]) continue;
        flood[np] = o|4;
        tally[o]++;
        queue[end++] = np;
      }
    }
    int score = tally[player]-tally[player^1];
    return score;
  }

  uint64_t ComputeHash() const;
};

void PrintField(const Field& field);

unique_ptr<Field> ParseField(const string& repr);

#endif
