#include <algorithm>
#include <chrono>
#include <iostream>
#include <memory>

#include "minimax.h"
#include "flags.h"
#include "interruption.h"
#include "random.h"
// #include "search_tree_printer.h"
#include "flags.h"
// #include "opening_table.h"
// #include "line_reader.h"
#include "hash_table.h"
#include "cmd_args.h"

using namespace std::chrono;

static const int HashMinDepth = 2;
static const int MinDepthSortMoves = 0;

// static const bool PrintSearchTree = false;

struct TimeLimitExceededException {
};

struct IntSignalException {
};

// unique_ptr<SearchTreePrinter> search_tree_printer_single;

// SearchTreePrinter* tree_printer() {
//   if (search_tree_printer_single == nullptr) {
//     search_tree_printer_single.reset(new SearchTreePrinter);
//   }
//   return search_tree_printer_single.get();
// }

// static const int MinDepthSortMoves = 2;

struct MoveSorter {
  inline bool operator()(int m1, int m2) const {
    // Implement some sort heuristic
    return m1 > m2;
  }
};

// inline void SortMoves(uint8_t* moves, int count) {
//   sort(moves, moves+count, MoveSorter());
// }

struct MoveScore {
  int dir;
  int score;

  inline bool operator<(const MoveScore& other) const {
    return score < other.score;
  }
};


// The MiniMax class contains all state being used when search the tree of moves.
class MiniMax {
 public:

   // Initialize the AI to search a specific field with the the given params
   // - field: the initial state of the field
   // - ply: how many moves have been playes so far in the game.
   // - time_limit: specifies the maximum amount of time to spend on the search in milliseconds, no time limit if zero.
   // - interruptable: whether the search can be interrupted if the is any data available in the input, used for pondering.
  MiniMax(const Field& field, int player, int ply, int time_limit_ms, int ponder, bool* interrupt_flag)
    : field(field),
      player(player),
      ply(ply),
      interrupt_flag(interrupt_flag),
      depth_shortening(*DepthShortening),
      shortening_threshold(*ShorteningThreshold)
      // pondering(ponder)
      {

    if (time_limit_ms == 0) {
      this->has_deadline = false;
    } else {
      auto now = steady_clock::now();
      this->deadline = now + milliseconds(time_limit_ms);
      this->has_deadline = true;
    }

    // if (PrintSearchTree) {
    //   printer = tree_printer();
    // }
  }

  // Search the best move from the given paramters given in the constructor.
  // It can throw exception TimeLimitExceeded if time spent exeeced the time
  // limit given in the constricutor.
  // It can throw exception InterrutionRequestedException if interruptable is
  // true and there is data available to be read from the input.
  // This functions is similar to DeepEval, the difference is that there is
  // some differences on the things that needs to be done on the root of the
  // search tree compared to the rest. Deduping repeated logic by refactoring
  // them out into functions or merging these functions proved to be less
  // efficient.
  SearchResult SearchMove(int depth) {
    this->depth = depth;
    // if (PrintSearchTree) {
    //   printer->Push(&field);
    //   printer->Attr("player", field.Turn());
    //   printer->Attr("ply", ply);
    //   printer->Attr("depth", depth);
    //   printer->Attr("field", field.BoardRepr());
    //   printer->Attr("macro", field.MacroBoardRepr());
    // }
    SearchResult out;
    out.score = -MAX_SCORE;
    out.depth = depth;
    out.move_count = 0;

    this->nodes++;

    //if (!pondering) {
      auto slot = TopLevelHashTableSingleton.Get(&field);
      if (slot != nullptr && slot->depth >= depth) {
        out.score = slot->score;
        out.move_count = slot->move_count;
        for (int i = 0; i < slot->move_count; i++) {
          out.moves[i] = slot->moves[i];
        }
        out.nodes = this->nodes;
        return out;
      }
    //}

    int first_move = -1;
    auto memo = HashTableSingleton.Get(&field);
    if (memo != nullptr) {
      // First level is ok to return a hash hit for higher depth if not pondering.
      // When pondering, we want to do a full search on all possible responses
      // if (/*!pondering &&*/ memo->depth >= depth) {
      //   if (memo->lower_bound == memo->upper_bound) {
      //     out.score = memo->lower_bound;
      //     out.moves[0] = memo->move;
      //     out.move_count = 1;
      //     out.nodes = this->nodes;
      //     return out;
      //   }
      // }
      first_move = memo->move;
    }

    int moves[4];
    int move_count = field.ValidMoves(player, moves);

    if (depth >= MinDepthSortMoves) {
      SortMoves(moves, move_count, first_move);
    }

    for (int i = 0; i < move_count; i++) {
      int move = moves[i];
      int alpha = (*AnalysisMode) ? -MAX_SCORE : out.score;
      int score = this->DeepEvalRec(moves[i], alpha, MAX_SCORE);
      if (score > out.score || out.move_count == 0) {
        out.score = score;
        out.moves[0] = move;
        out.move_count = 1;
      } else if (score == out.score) {
        out.moves[out.move_count++] = move;
      }
    }

    HashTableSingleton.Insert(&field, out.score, out.score, depth, out.moves[0]);
    TopLevelHashTableSingleton.Insert(&field, out.score, depth, out.moves, out.move_count);

    out.nodes = nodes;
    sort(out.moves, out.moves+out.move_count);

    // if (PrintSearchTree) {
    //   printer->Pop();
    // }
    return out;
  }

 private:

  // Checks whether the search should be interrupted, the appropriate exception
  // is thrown if so.
  inline void checkInterruption() {
    this->deadline_counter++;
    if (this->deadline_counter % (1<<14) != 0) {
      return;
    }
    if (interrupt_flag != nullptr && *interrupt_flag) {
      throw IntSignalException();
    }
    if (InterruptRequested()) {
      throw IntSignalException();
    }
    if (this->has_deadline) {
      auto now = steady_clock::now();
      if (now >= this->deadline) {
        throw TimeLimitExceededException();
      }
    }
  }

  // Recursive call for searching the tree, it updates all internal state to
  // search the next position and revert those updates at the end. This is done
  // to avoid copying or a very long list of function arguments that would be
  // less efficient.
  int DeepEvalRec(int move, int alpha, int beta) {
    this->nodes++;
    this->checkInterruption();

    auto save_hash = field.hash;
    field.MoveBot(player, move);
    int score;

    if (ply%2==1 && field.Over()) {
      int winner = field.Winner();
      if (winner == -1) {
        score = -1;
      } else if (winner == player) {
        score = MAX_SCORE - ply;
      } else {
        score = -MAX_SCORE+ply;
      }
    } else {
      depth--;
      ply++;
      player ^= 1;
      // if (PrintSearchTree) {
      //   printer->Push(&field);
      //   printer->Attr("player", field.Turn());
      //   printer->Attr("ply", ply);
      //   printer->Attr("depth", depth);
      //   printer->Attr("alpha", alpha);
      //   printer->Attr("beta", beta);
      //   printer->Attr("field", field.BoardRepr());
      //   printer->Attr("macro", field.MacroBoardRepr());
      // }
      bool full_search = true;
      if (depth_shortening > 0 && !shortened && depth >= depth_shortening /*&& !pondering*/) {
        shortened = true;
        depth -= depth_shortening;
        score = -this->DeepEval(-(beta+shortening_threshold+1), -(alpha-shortening_threshold-1));
        depth += depth_shortening;
        shortened = false;
        if (score < alpha - shortening_threshold || score > beta + shortening_threshold) {
          full_search = false;
        }
      }
      if (full_search) {
        // if (pondering) {
        //   pondering = false;
        //   // Perform a complete search if pondering.
        //   auto result = SearchMove();
        //   score = -result.score;
        //   pondering = true;
        // } else {
          score = -this->DeepEval(-beta, -alpha);
        //}
      }
      // if (PrintSearchTree) {
      //   printer->Attr("score", score);
      //   printer->Pop();
      // }
      depth++;
      ply--;
      player ^= 1;
    }
    field.UnmoveBot(player, move);
    ASSERT(save_hash == field.hash);
    return score;
  }

  // Performs the actual search in the move tree, it returns the score of the best move found.
  inline int DeepEval(int alpha, int beta) {

    {
      int upper_bound = MAX_SCORE - (ply + 1);
      if (upper_bound < alpha) {
        return upper_bound;
      }
      int lower_bound = -MAX_SCORE + (ply + 2);
      if (lower_bound > beta) {
        return lower_bound;
      }
    }

    int first_move = -1;
    if (depth >= HashMinDepth) {
      auto memo = HashTableSingleton.Get(&field);
      if (memo != nullptr) {
        // if (PrintSearchTree) {
        //   printer->Attr("hash_hit", true);
        //   printer->Attr("hash_lower_bound", memo->lower_bound);
        //   printer->Attr("hash_upper_bound", memo->upper_bound);
        // }
        if (memo->depth >= depth) {
          if (memo->lower_bound == memo->upper_bound) {
            return memo->lower_bound;
          }
          if (memo->lower_bound > beta) {
            return memo->lower_bound;
          }
          if (memo->upper_bound < alpha) {
            return memo->upper_bound;
          }
        }
        if (memo->lower_bound >= MAX_SCORE - 1000) {
          return memo->lower_bound;
        }
        if (memo->upper_bound <= -MAX_SCORE + 1000) {
          return memo->upper_bound;
        }
        first_move = memo->move;
      }
    }

    int move_count = 0;
    int moves[4];
    int best_score = -MAX_SCORE;

    if (depth <= 0) {
      // if (PrintSearchTree) {
      //   printer->Attr("leaf", true);
      // }
      best_score = field.Eval(player);
      if (best_score > beta) {
        return best_score;
      }
      move_count = 0;
    } else {
      move_count = field.ValidMoves(player, moves);
    }

    if (depth >= MinDepthSortMoves) {
      SortMoves(moves, move_count, first_move);
    }

    int best_move = -1;
    for (int i = 0; i < move_count; i++) {
      int move = moves[i];

      int score = this->DeepEvalRec(move, max(alpha, best_score+1), beta);
      if (score > best_score) {
        best_score = score;
        best_move = move;
        if (score > beta) {
          break;
        }
      }
    }
    // if (depth >= MinDepthSortMoves) {
    //   RewardMove(best_move);
    // }
    if (depth >= HashMinDepth) {
      int lower_bound = -MAX_SCORE;
      int upper_bound = MAX_SCORE;
      if (best_score > beta) {
        lower_bound = best_score;
      } else if (best_score < alpha) {
        upper_bound = best_score;
      } else {
        lower_bound = upper_bound = best_score;
      }
      HashTableSingleton.Insert(&field, lower_bound, upper_bound, depth, best_move);
    }
    return best_score;
  }


  inline void SortRegularMoves(int* moves, int count) {
    if (count <= 1) {
      return;
    }
    // cerr << "Before:" << endl;
    // for (int i = 0; i < count; i++) {
    //   cerr << moves[i] << " ";
    // }
    // cerr << endl;
    MoveScore scores[4];
    int pos = field.bots[player];
    for (int i = 0; i < count; i++) {
      int score = 0;
      int next = pos + moves[i];
      for (int dir = 0; dir < 4; dir++) {
        int n = next + dd[dir];
        if (!field.walls[n]) {
          score++;
        }
      }
      scores[i].score = score;
      scores[i].dir = moves[i];
    }
    stable_sort(scores, scores+count);
    for (int i = 0; i < count; i++) {
      moves[i] = scores[i].dir;
    }
    // cerr << "After:" << endl;
    // for (int i = 0; i < count; i++) {
    //   cerr << moves[i] << " ";
    // }
    // cerr << endl;
  }

  inline void SortMoves(int* moves, int count, int first_move) {
    if (first_move == -1 || count <= 1) {
      // SortRegularMoves(moves, count);
      return;
    }

    int first_move_idx = -1;
    for (int i = 0; i < count; i++) {
      if (moves[i] == first_move) {
        first_move_idx = i;
        break;
      }
    }
    if (first_move_idx == -1) {
      return;
    }
    for (int i = first_move_idx - 1; i >= 0; i--) {
      moves[i+1] = moves[i];
    }
    moves[0] = first_move;
    // SortRegularMoves(moves+1, count-1);
  }

  int64_t nodes = 0;
  int deadline_counter = 0;
  steady_clock::time_point deadline;
  bool has_deadline;
  Field field;
  int player;
  int ply = 0;
  int depth = 0;
  bool* interrupt_flag;
  bool shortened = false;

  const int depth_shortening;
  const int shortening_threshold;

  // bool pondering;

  // SearchTreePrinter* printer;

};

std::ostream& operator<<(std::ostream& stream, const SearchResult& res) {
  stream << "Move: [";
  for (int i = 0; i < res.move_count; i++) {
    if (i > 0) {
      stream << ", ";
    }
    stream << move_names[res.moves[i]];
  }
  stream << "] Score: " << res.score << " Depth: " << res.depth << " Nodes: " << HumanReadable(res.nodes) << " Time: " << res.search_time_ms << "ms";
  return stream;
}

int SearchResult::RandomMove() const {
  if (move_count > 0) {
    return moves[RandN(move_count)];
  } else {
    return LEFT;
  }
}

bool checkOpeningHeuristic(const Field& field, int player) {
  int y1 = field.bots[player] / WIDTH;
  int x1 = field.bots[player] % WIDTH;
  int count[2] = {0, 0};
  int side = (x1 - 1)/8;
  if (side != player) {
    return false;
  }
  if (x1 == 8 || x1 == 9) {
    return false;
  }
  for (int y = 1; y < 17; y++) {
    if (y == y1) continue;
    for (int x = 1; x < 17; x++) {
      if (field.walls[x+y*WIDTH]) {
        int p = (x - 1) / 8;
        count[p]++;
      }
    }
  }
  if (count[player] > 0) {
    return false;
  }
  return true;
}

// Compute the best move for the given field and player.
// This is the entry point for the AI search.
SearchResult SearchMove(const Field &field, int player, SearchOptions opt, bool* interrupt_flag) {
  SearchResult out;

  // Lookup opening table, return a move from there if we find a hit.
  // if (*EnableOpeningTable && opt.use_open_table) {
  //   auto item = FindOpeningTable(*field);
  //   if (item) {
  //     out.nodes = 0;
  //     out.depth = 0;
  //     out.score = 0;
  //     out.move_count = 0;
  //     for (int i = 0; i < item->move_count; i++) {
  //       int m = item->moves[i];
  //       if (!field->canTick(m)) {
  //         cerr << "Invalid move from opening table: " << m << endl;
  //         continue;
  //       }
  //       out.moves[out.move_count++] = m;
  //     }
  //     if (out.move_count > 0) {
  //       cerr << "Opening table hit" << endl;
  //       return out;
  //     }
  //   }
  // }

  if (checkOpeningHeuristic(field, player) && !*AnalysisMode) {
    if (player == 0) {
      out.moves[0] = RIGHT;
    } else {
      out.moves[0] = LEFT;
    }
    out.nodes = 0;
    out.depth = 0;
    out.score = 0;
    out.move_count = 1;
    return out;
  }

  MiniMax mm(field, player, field.ply, opt.time_limit_ms, opt.pondering, interrupt_flag);
  out.move_count = 0;
  auto start = steady_clock::now();
  
  // Iterative deepening search. It is better than fixed deptch because of time
  // constraints, ie, fixed depth can take a variable amount of depth. It is
  // also better to populate the cache with good moves, which improves
  // alpha-beta search.
  for (int depth = 2; depth <= *MaxDepth; depth +=2) {
    SearchResult tmp;
    try {
      tmp = mm.SearchMove(depth);
      tmp.search_time_ms = duration_cast<milliseconds>(steady_clock::now() - start).count();
    } catch (TimeLimitExceededException e) {
      cerr << "Search interrupted after reaching time limit of " << opt.time_limit_ms << " milliseconds" << endl;
      out.time_limit_exceeded = true;
      break;
    } catch (IntSignalException e) {
      cerr << "Search manually interrupted" << endl;
      out.signal_interruption = true;
      break;
    }
    cerr << tmp;
    cerr << endl;
    out = tmp;
  }

  // if (*AnalysisMode) {
  //   try {
  //     Field field_copy(field);
  //     for (int i = 0; i < out.depth; i++) {
  //       MiniMax mm(field_copy, player, field.ply, opt.time_limit_ms, false, nullptr);
  //       auto tmp = mm.SearchMove(out.depth - i);
  //       cerr << tmp << endl;
  //       field_copy.MoveBot(player, tmp.RandomMove());
  //       player ^= 1;
  //       PrintField(field_copy);
  //     }
  //   } catch (TimeLimitExceededException e) {
  //   } catch (IntSignalException e) {
  //   }
  // }
  return out;
}
