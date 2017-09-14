#ifndef MINIMAX_H
#define MINIMAX_H

#include <iostream>

#include "field.h"

struct SearchResult {
  int moves[4]; // All best moves
  int move_count = 0; // Number of best moves
  int score = 0; // Score of the best move.
  int64_t nodes = 0; // Number of postiions analysed.
  int depth = 0; // Maximum depth that was fully analysed.
  bool time_limit_exceeded = false; // Whether time limit was exceeded before finishing the search or reaching maximum depth.
  bool signal_interruption = false; // Whether the search was interrupted because it received sigint.
  bool input_interruption = false; // Whether the search was interrupted because there was data available in the input.

  int RandomMove() const; // Returns one of the best moves randomly.
};

struct SearchOptions {
  bool use_open_table = true;
  bool interruptable = false;
  int time_limit_ms = 100;
  bool pondering = false;
};

std::ostream& operator<<(std::ostream&, const SearchResult&);

SearchResult SearchMove(const Field *board, SearchOptions opt = SearchOptions());

#endif
