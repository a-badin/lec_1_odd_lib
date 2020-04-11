#include <unistd.h>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <vector>
#include <memory>
#include <ratio>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>

#include "utils.hpp"

using namespace utils;
using namespace std::chrono;

constexpr size_t ITERATIONS = 5000000;

int open_file(const std::string& path) {
  int fd = ::open(path.c_str(), O_RDONLY);
  throw_on_error(fd, "Error opening file");
  return fd;
}

size_t get_file_length(int file) {
  off_t len = lseek(file, 0, SEEK_END);
  throw_on_error(len, "Error getting file length");
  return static_cast<size_t>(len);
}

std::vector<size_t> get_random_seeks(size_t max_seek) {
  std::vector<size_t> seeks;
  for (int i = 0; i < ITERATIONS; ++i) {
    seeks.push_back(std::rand() % (max_seek - 1));
  }
  return seeks;
}

int main(int argc, char** argv) {
  if (argc < 2) {
    std::cerr << "Usage: mmap FILE" << std::endl;
    return 1;
  }
  int fd = open_file(argv[1]);
  size_t length = get_file_length(fd);
  std::cout << "Size is " << length << std::endl;

  std::vector<size_t> seeks = get_random_seeks(length);

  size_t summ = 0;
  auto start = high_resolution_clock::now();
  for (size_t seek : seeks) {
    char r;
    lseek(fd, seek, SEEK_SET);
    throw_on_error(::read(fd, &r, 1), "Error reading");
    summ += static_cast<uint8_t>(r);
  }
  auto elapsed = duration<double>(high_resolution_clock::now() - start);
  std::cerr << "Traditional took " << elapsed.count() << " summ:" << summ << std::endl;
  summ = 0;

  void* mmap = ::mmap(nullptr, length, PROT_READ, MAP_SHARED, fd, 0);

  // char* g = static_cast<char*>(mmap);
  // *g = 10; // SEGFAULT

  if (mmap == MAP_FAILED)
    throw utils::Exception("Error mmaping"); 
  char* buf = static_cast<char*>(mmap);
  start = high_resolution_clock::now();
  for (size_t seek : seeks) {
    char* r = buf + seek;
    summ += static_cast<uint8_t>(*r);
  }
  elapsed = duration<double>(high_resolution_clock::now() - start);
  std::cerr << "MMaped took " << elapsed.count() << " summ:" << summ << std::endl;
  ::munmap(mmap, length);
  return 0;
}
