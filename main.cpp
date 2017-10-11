#include <algorithm>
#include <cassert>
#include <ctime>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
#include <chrono>
#include <map>

#include "cmd_args.h"
#include "field.h"
#include "flags.h"
#include "hash_table.h"
#include "heuristic.h"
#include "interruption.h"
#include "minimax.h"
#include "minimax_worker.h"
#include "random.h"

using namespace std;
using namespace std::chrono;

struct Settings {
  int timebank = 10000;
  int time_per_move = 500;
  string player_names;
  string my_name;
  int my_id = 0;

  int field_width = 20;
  int field_height = 14;

  bool handleSettings(istream& stream) {
		string type;
		stream >> type;
		if (type == "timebank") {
			return bool(stream >> timebank);
		} else if (type == "time_per_move") {
			return bool(stream >> time_per_move);
		} else if (type == "player_names") {
			return bool(stream >> player_names);
			// player names aren't very useful
		} else if (type == "your_bot") {
			return bool(stream >> my_name);
		} else if (type == "your_botid") {
			return bool(stream >> my_id);
		} else if (type == "field_width") {
			if (!(stream >> field_width)) {
        return false;
      }
      if (field_width != 16) {
        cerr << "Field size not supported" << endl;
        return false;
      }
		} else if (type == "field_height") {
			if (!(stream >> field_height)) {
        return false;
      }
      if (field_height != 16) {
        cerr << "Field size not supported" << endl;
        return false;
      }
    } else {
      return false;
		}
    return true;
  }
};

struct Game {
  int round = 0;
  string field_repr;

  Settings settings;

  HeuristicWeights heuristic_weights;

  // Worker worker;

  Game() {
    // worker.Init();
  }

  ~Game() {
    // worker.Close();
  }

  bool handleUpdate(istream& stream) {
		string player_name, type;
		stream >> player_name >> type;
		if (type == "round") {
			return bool(stream >> round);
		} else if (type == "field") {
      return bool(stream >> field_repr);
    } else {
      return false;
		}
    return true;
  }

  int pickRandomMove(const Field& field) {
    int moves[4];
    // Just pick a random move
    int n = field.ValidMoves(settings.my_id, moves);
    int move = 0;
    if (n > 0) {
      int total = 0;
      for (int i = 0; i < n; i++) {
        total += heuristic_weights.dir_weights[moves[i]];
      }
      int r = RandN(total);
      for (int i = 0; i < n; i++) {
        r -= heuristic_weights.dir_weights[moves[i]];
        if (r < 0) {
          move = moves[i];
          break;
        }
      }
      ASSERT(r < 0);
    }
    return move;
  }

  bool handleAction(int time_remaining_ms) {
    auto field = ParseField(field_repr);
    if (!field) {
      return false;
    }
    PrintField(*field);

    int time_to_move_ms = time_remaining_ms / (MAX_ROUNDS - round + 1) + settings.time_per_move;
    int time_limit_ms = max(1, min(time_to_move_ms, time_remaining_ms - 25));

    SearchOptions opts;
    if (*AnalysisMode) {
      opts.time_limit_ms = 10000000;
    } else {
      cerr << "Time remaining: " << time_remaining_ms << '\n';
      cerr << "Time limit: " << time_limit_ms << '\n';
      opts.time_limit_ms = time_limit_ms;
    }
    // worker.Start(*field, settings.my_id, opts);
    // auto result = worker.Wait();
    auto result = SearchMove(*field, settings.my_id, opts);

    if (result.signal_interruption && !*AnalysisMode) {
      return false;
    }

    int move = result.RandomMove();

    cout << move_names[move] << endl << flush;
    return true;
  }

  bool handleFeatures() {
    auto field = ParseField(field_repr);
    if (!field) {
      return false;
    }
    cout << field->gen_features() << endl << flush;
    return true;
  }

  bool processNextCommand(const string& line) {
    stringstream stream(line);
    string command;
    if (!(stream >> command)) {
      return true;
    }
    if (command == "settings") {
      return settings.handleSettings(stream);
    } else if (command == "update") {
      handleUpdate(stream);
    } else if (command == "action") {
      int time_remaining;
      string useless_move;
      if (!(stream >> useless_move)) {
        return false;
      }
      if (useless_move != "move") {
        return false;
      }
      if (!(stream >> time_remaining)) {
        return false;
      }
      return handleAction(time_remaining);
    } else if(command == "weights") {
      map<string, int> values;
      int value;
      string name;
      cerr << "Updating weights:";
      while (stream >> name >> value) {
        values[name] = value;
        cerr << ' ' << name << ": " << value;
      }
      cerr << '\n';
      HeuristicWeights weights;
      weights.from_map(values);
      heuristic_weights = weights;
      return true;
    } else if(command == "features") {
      return handleFeatures();
    } else {
      return false;
    }
    return true;
  }
};

int main(int argc, const char** argv) {
  std::ios_base::sync_with_stdio(false);
  std::setvbuf(stdout, nullptr, _IOFBF, BUFSIZ);

  auto seed = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
  cerr << "Seed: " << seed << endl;
  RandSeed(seed);

  if (!ParseFlags(argc, argv)) {
    return 1;
  }
  InitHashConstants();
  HashTableSingleton.Init(*HashSize);
  InitSignals();
  Game game;
  string line;
  fstream tee;
  if (!TeeInput->empty()) {
    tee.open(*TeeInput, iostream::out);
  }

  cerr << "Bot ready to process input" << endl;
  while (getline(cin, line)) {
    if (tee.good()) {
      tee << line << '\n' << flush;
    }
    if (!game.processNextCommand(line)) {
      cerr << "Failed to process command: " << line << endl;
      return 1;
    }
	}
  HashTableSingleton.PrintStats();
	return 0;
}
