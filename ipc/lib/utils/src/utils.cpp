#include "utils.hpp"

namespace utils {

Exception::Exception(const std::string& msg)
  : msg_{msg} {}

const char * Exception::what() const noexcept {
  return msg_.c_str();
}

void throw_on_error(int res, const std::string& msg) {
  if (res < 0) {
    throw Exception(msg + " " + std::strerror(errno));
  }
}

} // namespace utils
