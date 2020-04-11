#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <signal.h>
#include <sstream>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>

#include "utils.hpp"

const std::string LOG_PATH = "log.txt";

using utils::throw_on_error;
using namespace std::literals;

void print_time(std::ostream& stream) {
  std::stringstream ss;
  auto now = std::chrono::system_clock::now();
  std::time_t time = std::chrono::system_clock::to_time_t(now);
  ss << "#" << getpid() << " " << std::put_time(std::localtime(&time), "%T") << std::endl;
  stream << ss.str();
}

void reopen_log(std::ofstream &stream) {
  std::ofstream f;
  f.rdbuf()->pubsetbuf(nullptr, 0);
  using std::ios_base;
  f.open(LOG_PATH, ios_base::out | ios_base::app);
  if (!f)
    throw utils::Exception("Error opening log file");
  stream = std::move(f);
}

void rotate_log() {
  std::stringstream ss;
  auto now = std::chrono::system_clock::now();
  std::time_t time = std::chrono::system_clock::to_time_t(now);
  ss << std::put_time(std::localtime(&time), "log_%F.txt");
  int res = ::rename(LOG_PATH.c_str(), ss.str().c_str());
  throw_on_error(res, "Error renaming");
}

sig_atomic_t GLOBAL_LOG_ROTATE = 0;
sig_atomic_t GLOBAL_SHUT_DOWN = 0;

void handler(int signal) {
  if (signal == SIGHUP) {
    GLOBAL_LOG_ROTATE = 1;
  } else if (signal == SIGINT) {
    GLOBAL_SHUT_DOWN = 1;
  }
}

void set_sig_handler() {
  struct sigaction sa{};
  sa.sa_handler = &handler;

  for (int signal : {SIGHUP, SIGINT}) {
    int res = sigaction(SIGHUP, &sa, nullptr);
    throw_on_error(res, "Error setting SIGHUP handler");
  }
}

int main(int argc, char** argv) {
  set_sig_handler();
  std::cerr << "Call: 'kill -SIGHUP " << getpid() << "' to rotate log" << std::endl;

  int child_pid = fork();
  throw_on_error(child_pid, "Error creating fork");

  std::ofstream log;
  reopen_log(log);

  if (child_pid > 0) { // Parent
    while (!GLOBAL_SHUT_DOWN) {
      std::this_thread::sleep_for(2s);
      print_time(log);
      if (GLOBAL_LOG_ROTATE) {
        GLOBAL_LOG_ROTATE = 0;
        rotate_log();
        reopen_log(log);
        kill(child_pid, SIGHUP);
      }
    }
    kill(child_pid, SIGINT);
  } else { // Child
    while (!GLOBAL_SHUT_DOWN) {
      std::this_thread::sleep_for(1s);
      print_time(log);
      if (GLOBAL_LOG_ROTATE) {
        GLOBAL_LOG_ROTATE = 0;
        reopen_log(log);
      }
    }
  }

  int status;
  ::waitpid(child_pid, &status, 0);
  return status;
}
