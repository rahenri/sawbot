#ifndef UTILS_H
#define UTILS_H

#include <vector>
#include <string>

using namespace std;

extern string move_names[];

vector<string> Split(const string& str, char sep);

#ifdef _LOCAL
#  define ASSERT(COND) do{if(!(COND)){cerr << #COND << endl;int*x=nullptr;*x=1;}}while(false)
#else
#  define ASSERT(COND) do{}while(false)
#endif

template <class T>
string HumanReadable(T number) {
  if (number == 0) {
    return "0";
  }
  bool neg = number < 0;
  if (neg) number = -number;
  string output;
  for (int i = 0; number > 0; i++) {
    if (i % 3 == 0 && i > 0) {
      output = "," + output;
    }
    output = char(number%10 + '0') + output;
    number /= 10;
  }
  return output;
}

#endif
