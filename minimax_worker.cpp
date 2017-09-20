#include <cassert>

#include "minimax_worker.h"

void Worker::Init() {
  cerr << "Starting worker thread..." << endl;
  thread_ = unique_ptr<thread>(new thread(&Worker::static_worker, this));
}

void Worker::static_worker(Worker* worker) {
  cerr << "Worker thread started" << endl;
  worker->worker();
}

void Worker::worker() {
  while(1) {
    unique_lock<mutex> lk(mutex_);
    while (1) {
      if (ready_) {
        break;
      }
      if (closing_) {
        return;
      }
      worker_condition_.wait(lk);
    }
    running_ = true;
    Field board = board_;
    int player = player_;
    SearchOptions opt = opt_;
    interrupt_flag_ = false;
    done_ = false;

    board_ = Field();
    player_ = 0;
    opt_ = SearchOptions();
    lk.unlock();

    auto result = SearchMove(board, player, opt, &interrupt_flag_);

    unique_lock<mutex> lk2(mutex_);
    running_ = false;
    ready_ = false;
    done_ = true;
    last_result_ = result;
    client_condition_.notify_all();

    lk2.unlock();
  }
}

void Worker::Close() {
  unique_lock<mutex> lk(mutex_);
  closing_ = true;
  lk.unlock();
  worker_condition_.notify_all();

  thread_->join();
}

void Worker::Interrupt() {
  interrupt_flag_ = true;
}

SearchResult Worker::Wait() {
  unique_lock<mutex> lk(mutex_);
  while(running_ || ready_) {
    client_condition_.wait(lk);
  }
  assert(done_);
  auto result = last_result_;
  last_result_ = SearchResult();
  done_ = false;
  lk.unlock();

  return result;
}

void Worker::Start(const Field& board, int player, SearchOptions opt) {
  unique_lock<mutex> lk(mutex_);
  assert(!running_);

  board_ = board;
  player_ = player;
  opt_ = opt;
  ready_ = true;
  lk.unlock();
  worker_condition_.notify_all();
}

bool Worker::isRunning() {
  unique_lock<mutex> lk(mutex_);
  return running_ || ready_ || done_;
}
