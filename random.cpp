#include <random>

std::default_random_engine generator;

void RandSeed(int64_t seed) {
  generator.seed(seed);
}
