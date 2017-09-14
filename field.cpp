#include <algorithm>
#include <cassert>
#include <random>

#include "field.h"
#include "random.h"

uint64_t bot_hash[2][FIELD_SIZE];
uint64_t field_hash[FIELD_SIZE];
uint64_t base_hash;

void PrintField(const Field& field) {
  char field_str[HEIGHT][WIDTH+2];
  for (int y = 0; y < HEIGHT; y++) {
    for (int x = 0; x < WIDTH; x++) {
      int pos = (x) + (y)*WIDTH;
      char c = ' ';
      if ((y == 0 or y == HEIGHT-1) and (x == 0 or x == WIDTH-1)) {
        c = '+';
      } else if (y == 0 or y == HEIGHT-1) {
        c = '-';
      } else if (x == 0 or x == WIDTH-1) {
        c = '|';
      } else {
        if (field.walls[pos]) {
          c = '#';
        }
      }
      field_str[y][x] = c;
    }
    field_str[y][WIDTH] = '\n';
    field_str[y][WIDTH+1] = 0;
  }
  for (int i = 0; i < 2; i++) {
    int p = field.bots[i];
    int y = p/WIDTH;
    int x = p%WIDTH;
    field_str[y][x] = '0' + i;
  }
  cerr << "Bot1 pos: " << field.bots[0] << " Bot2 pos:" << field.bots[1] << '\n';
  for (int y = 0; y < HEIGHT; y++) {
    cerr << field_str[y];
  }
}

unique_ptr<Field> ParseField(const string& repr) {
  auto parts = Split(repr, ',');

  ASSERT(int(parts.size()) == (WIDTH-2)*(HEIGHT-2));
  unique_ptr<Field> field(new Field());
  for (int i = 0; i < int(parts.size()); i++) {
    int pos = i + WIDTH + 1 + 2 * (i / (WIDTH-2));
    field->walls[pos] = 0;
    if (parts[i] == ".") {
      field->walls[pos] = 0;
    } else if (parts[i] == "X" || parts[i] == "x") {
      field->walls[pos] = 1;
    } else if (parts[i] == "0") {
      field->bots[0] = pos;
      field->walls[pos] = 1;
    } else if (parts[i] == "1") {
      field->bots[1] = pos;
      field->walls[pos] = 1;
    } else {
      cerr << "Invalid field: " << parts[i] << endl;
      return nullptr;
    }
  }
  field->hash = field->ComputeHash();
  return field;
}

void InitHashConstants() {
  mt19937_64 generator;
  generator.seed(0x438ead462650e4a);
  base_hash = generator();
  for (int i = 0; i < FIELD_SIZE; i++) {
    for (int j = 0; j < 2; j++) {
      bot_hash[j][i] = generator();
    }
  }
  for (int i = 0; i < FIELD_SIZE; i++) {
    field_hash[i] = generator();
  }
}

uint64_t Field::ComputeHash() const {
  uint64_t output = base_hash;

  for (int i = 0; i < 2; i++) {
    output ^= bot_hash[i][bots[i]];
  }

  for (int i = 0; i < FIELD_SIZE; i++) {
    if (walls[i] == 1) {
      output ^= field_hash[i];
    }
  }

  return output;
}
