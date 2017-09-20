#ifndef MINIMAX_WORKER_H
#define MINIMAX_WORKER_H

#include <thread>
#include <mutex>
#include <condition_variable>

#include "minimax.h"

class Worker {
  public:
    void Init();

    void Start(const Field& board, int player, SearchOptions opt);

    SearchResult Wait();

    void Interrupt();

    void Close();

    bool isRunning();
  private:

    void worker();

    thread thread_;

    bool running_ = false;

    bool ready_ = false;

    bool closing_ = false;

    bool interrupt_flag_ = false;

    bool done_ = false;

    mutex mutex_;

    condition_variable worker_condition_;
    condition_variable client_condition_;

    SearchResult last_result_;

    Field board_;

    int player_;

    SearchOptions opt_;
};

#endif
