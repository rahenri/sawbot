#include <vector>
#include <string>

#include "utils.h"

using namespace std;

string move_names[] = { "right", "down", "left", "up", "pass" };

vector<string> Split(const string& str, char sep) {
  vector<string> out;
  string partial;
  for (char c : str) {
    if (c == sep) {
      out.push_back(partial);
      partial = "";
    } else {
      partial += c;
    }
  }
  out.push_back(partial);
  return out;
}
