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

#endif
