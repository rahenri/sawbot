#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include <cstdint>
#include <unordered_map>
#include <cstring>

#include "field.h"
#include "flags.h"

using namespace std;

struct BoardMemo {
  uint64_t hash;
  int32_t lower_bound, upper_bound;
  int8_t depth;
  int8_t move;
} __attribute__((packed));

struct TopLevelSlot {
  int32_t score = 0;
  int depth = -1;
  int moves[81];
  int move_count = 0;
};

class HashTable {
  public:
    ~HashTable() {
      if (this->data) {
        delete[] this->data;
      }
    }
    void Init(int size) {
      cerr << "Initializing hash table with " << (size * sizeof(BoardMemo)/1024/1024) << "MB" << endl;
      this->size = size;
      this->data = new BoardMemo[size];
      memset(this->data, 0, sizeof(BoardMemo) * size);
      cerr << "Done initializing hash table" << endl;
    }

    BoardMemo* Get(const Field* board) {
      lookups++;
      uint64_t hash = board->hash;
      BoardMemo* memo = this->data + (hash % size);
      if (memo->hash != hash) {
        return nullptr;
      }
      hits++;
      return memo;
    }

    BoardMemo* Insert(const Field *board, int lower_bound, int upper_bound, int depth, int move) {
      inserts++;
      uint64_t hash = board->hash;
      BoardMemo* memo = this->data + (hash % size);
      if (memo->hash != hash || depth > memo->depth) {
        if (memo->hash != 0 && memo->hash != hash) {
          evictions++;
        }
        memo->hash = hash;
        memo->depth = depth;
        memo->lower_bound = lower_bound;
        memo->upper_bound = upper_bound;
        memo->move = move;
      } else if(memo->hash == hash && depth == memo->depth) {
        if (lower_bound > memo->lower_bound) {
          memo->move = move;
        }
        memo->lower_bound = max(memo->lower_bound, lower_bound);
        memo->upper_bound = min(memo->upper_bound, upper_bound);
      }
      return memo;
    }

    double Utilization() {
      int used = 0;
      for (int i = 0; i < size; i++) {
        if (data[i].hash != 0) {
          used ++;
        }
      }
      return double(used) / double(size);
    }

    void PrintStats() {
      cerr << "Hash utilization: " << (Utilization() * 100) << "%" << endl;
      cerr << "Hits: " << HumanReadable(hits) << endl;
      cerr << "lookups: " << HumanReadable(lookups) << endl;
      cerr << "evictions: " << HumanReadable(evictions) << endl;
      cerr << "inserts: " << HumanReadable(inserts) << endl;
      cerr << "Cache hit rate: " << (double(hits)/double(lookups)*100.0) << "%" << endl;
    }

  private:
    BoardMemo* data = nullptr;
    int size = 0;

    int64_t hits = 0;
    int64_t evictions = 0;
    int64_t lookups = 0;
    int64_t inserts = 0;
};

class TopLevelHashTable {
  public:
    TopLevelSlot* Get(const Field* board) {
      uint64_t hash = board->hash;
      auto it = data.find(hash);
      if (it == data.end()) {
        return nullptr;
      }
      TopLevelSlot* slot = &(it->second);
      return slot;
    }

    TopLevelSlot* Insert(const Field *board, int score, int depth, int* moves, int move_count) {
      TopLevelSlot* slot = Get(board);
      if (slot == nullptr) {
        auto it = data.insert(make_pair(board->hash, TopLevelSlot()));
        slot = &(it.first->second);
      }
      if (depth > slot->depth) {
        slot->depth = depth;
        slot->score = score;
        slot->move_count = move_count;
        for (int i = 0 ; i < move_count; i++) {
          slot->moves[i] = moves[i];
        }
      }
      return slot;
    }

  private:
    unordered_map<uint64_t, TopLevelSlot> data;
};

extern HashTable HashTableSingleton;

extern TopLevelHashTable TopLevelHashTableSingleton;

#endif
