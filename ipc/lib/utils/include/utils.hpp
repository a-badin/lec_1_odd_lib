#pragma once
#include <cstring>
#include <string>
#include <exception>

namespace utils {

class Exception : public std::exception {
public:
  Exception(const std::string& msg);
  const char * what() const noexcept override;

private:
  std::string msg_;
};

void throw_on_error(int res, const std::string& msg);

} // namespace utils
