#include <atomic>
#include <cstring>
#include <iostream>
#include <signal.h>

#include "interruption.h"

using namespace std;

volatile sig_atomic_t interruption_requested = 0;

void HandleInterrupt(int sig) {
  interruption_requested++;
}

void HandleSigsev(int sig) {
  cerr << "Seg fault!\n";
  exit(1);
}

bool InterruptRequested() {
  if (interruption_requested > 0) {
    interruption_requested--;
    return true;
  }
  return false;
}

void InitSignals() {
  signal(SIGSEGV, HandleSigsev);

  signal(SIGINT, HandleInterrupt);
}
