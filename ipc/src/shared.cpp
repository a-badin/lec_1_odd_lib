#include <unistd.h>
#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iterator>
#include <list>
#include <vector>
#include <memory>
#include <ratio>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <sys/mman.h>
#include <semaphore.h>
#include <functional>

#include "utils.hpp"

using namespace utils;
using namespace std::literals;

constexpr size_t MMAP_SIZE = 4096;

template<typename T>
using ShUniquePtr = std::unique_ptr<T, std::function<void(T*)>>;

template<typename T>
ShUniquePtr<T> make_shmem() {
  void* mmap = ::mmap(nullptr, sizeof(T),
                      PROT_WRITE | PROT_READ,
                      MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  if (mmap == MAP_FAILED)
    throw Exception("Error creaing mmap");

  return {reinterpret_cast<T*>(mmap), [](T* t) { ::munmap(t, sizeof(T)); }};
}


int main(int argc, char** argv)
{
  ShUniquePtr<int> sh_int = make_shmem<int>();
  *sh_int = 0;

  int child = fork();
  if (child) {
    *sh_int = 10;
  } else {
    std::this_thread::sleep_for(1s);
    std::cerr << *sh_int << std::endl;
    return 0;
  }

  waitpid(child, nullptr, 0);
  return 0;
}

