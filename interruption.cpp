#include <atomic>
#include <cstring>
#include <iostream>
#include <signal.h>

#include "interruption.h"

using namespace std;

volatile sig_atomic_t interruption_requested = 0;

static void HandleInterrupt(int sig) {
  interruption_requested++;
}

static void HandleSigsev(int sig) {
  cerr << "Crashed!" << endl;
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
