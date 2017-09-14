#include <algorithm>
#include <cassert>
#include <ctime>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
#include <map>

#include "cmd_args.h"
#include "field.h"
#include "flags.h"
#include "random.h"
#include "heuristic.h"

using namespace std;

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
  int snippets[2] = {0, 0};
  bool has_weapon[2] = {false, false};
  bool is_paralyzed[2] = {false, false};

  Settings settings;

  HeuristicWeights heuristic_weights;

  Game() { }

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

  bool handleAction(int time_remaining) {
    auto field = ParseField(field_repr);
    if (!field) {
      return false;
    }
    PrintField(*field);

    int time_to_move = time_remaining / (MAX_ROUNDS - round + 1) + settings.time_per_move;
    int time_limit = min(time_to_move, time_remaining - 25);
    cerr << "Time remaining: " << time_remaining << '\n';
    cerr << "Time limit: " << time_limit << '\n';

    int moves[4];
    // Just pick a random move
    int n = field->ValidMoves(settings.my_id, moves);
    int mid = 0;
    if (n > 0) {
      int total = 0;
      for (int i = 0; i < n; i++) {
        total += heuristic_weights.dir_weights[moves[i]];
      }
      int r = RandN(total);
      for (int i = 0; i < n; i++) {
        r -= heuristic_weights.dir_weights[moves[i]];
        if (r < 0) {
          mid = moves[i];
          break;
        }
      }
      ASSERT(r < 0);
    }

    cout << move_names[mid] << endl << flush;
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

    } else {
      return false;
    }
    return true;
  }
};

int main(int argc, const char** argv) {
  std::ios_base::sync_with_stdio(false);
  std::setvbuf(stdout, nullptr, _IOFBF, BUFSIZ);

  if (!ParseFlags(argc, argv)) {
    return 1;
  }
  InitHashConstants();
  Game game;
  string line;
  fstream tee;
  if (!TeeInput->empty()) {
    tee.open(*TeeInput, iostream::out);
  }
  while (getline(cin, line)) {
    if (tee.good()) {
      tee << line << '\n' << flush;
    }
    if (!game.processNextCommand(line)) {
      cerr << "Failed to process command: " << line << endl;
      return 1;
    }
	}
	return 0;
}
