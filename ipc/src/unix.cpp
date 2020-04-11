#include <errno.h>
#include <iterator>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <thread>
#include <unistd.h>
#include <iostream>
#include <cmath>


#include "utils.hpp"

using namespace std::literals;

/*
 * 6c 02 01 01
 * 0c 00 00 00 // Length 12
 * 01 00 00 00 // serial
 * 3d 00 00 00 // Array Length in bytes 61
 * 06 01 73s 00(07 00 00 00)3a 31 2e 31 33 31 36 00 // // Destination  :1.1316 (16)
 * 05 01 75u 00 01 00 00 00 // Reply serial
 * 08 01 67g 00 01 73 00 00 // Signature
 * 07 01 73s 00(14 00 00 00)6f 72 67 2e 66 72 65 65 64 65 73 6b 74 6f 70 2e 44 42 75 73 00 // org.freedesktop.DBus (61) //Sender
   Header
 * 00 00 00 // padding
 * (07 00 00 00)3a 31 2e 31 33 31 36 00
 *
*/
const std::string HELLO_MESSAGE = "l\1\0\1\0\0\0\0\1\0\0\0m\0\0\0\1\1o\0\25\0\0\0/org/"
                                  "freedesktop/DBus\0\0\0\3\1s\0\5\0\0\0Hello\0\0\0\2"
                                  "\1s\0\24\0\0\0org.freedesktop.DBus\0\0\0\0\6\1s\0\24"
                                  "\0\0\0org.freedesktop.DBus\0\0\0\0"s;

const std::string ADD_MATCH_1_MESSAGE = "l\1\0\1\211\0\0\0\2\0\0\0}\0\0\0\10\1g\0\1s\0"
                                        "\0\1\1o\0\25\0\0\0/org/freedesktop/DBus\0\0\0"
                                        "\3\1s\0\10\0\0\0AddMatch\0\0\0\0\0\0\0\0\2\1s"
                                        "\0\24\0\0\0org.freedesktop.DBus\0\0\0\0\6\1s"
                                        "\0\24\0\0\0org.freedesktop.DBus\0\0\0\0\204\0"
                                        "\0\0type='signal',sender='org.freedesktop.Notifications'"
                                        ",interface='org.freedesktop.Notifications',"
                                        "path='/org/freedesktop/Notifications'\0"s;

const std::string ADD_MATCH_2_MESSAGE = "l\1\0\1\255\0\0\0\3\0\0\0}\0\0\0\10\1g\0\1s\0"
                                        "\0\1\1o\0\25\0\0\0/org/freedesktop/DBus\0\0\0"
                                        "\3\1s\0\10\0\0\0AddMatch\0\0\0\0\0\0\0\0\2\1s"
                                        "\0\24\0\0\0org.freedesktop.DBus\0\0\0\0\6\1s"
                                        "\0\24\0\0\0org.freedesktop.DBus\0\0\0\0\250\0"
                                        "\0\0type='signal',sender='org.freedesktop.DBus'"
                                        ",interface='org.freedesktop.DBus',member='NameOwnerChanged'"
                                        ",path='/org/freedesktop/DBus',arg0='org.freedesktop.Notifications'\0"s;


const std::string NOTIFICATION_MESSAGE = "l\1\0\1\\\0\0\0\7\0\0\0\206\0\0\0\10\1g\0\rsusssasa{sv}i\0\0\0\0\0\0\1"
                                         "\1o\0\36\0\0\0/org/freedesktop/Notifications\0\0\3\1s\0\6\0\0\0Notify"
                                         "\0\0\2\1s\0\35\0\0\0org.freedesktop.Notifications\0\0\0\6\1s\0\5\0\0\0"
                                         ":1.21\0\0\0\v\0\0\0notify-send\0\0\0\0\0\0\0\0\0\0\0\0\0\6\0\0\0AdvCpp"
                                         "\0\0\f\0\0\0Hello World!\0\0\0\0\0\0\0\0\20\0\0\0\0\0\0\0\7\0\0\0urgency\0\1y\0\1\377\377\377\377"s;


int GLOBAL_SOCKET = -1;
const std::string SOCKET_NAME = "/run/user/1000/bus";

constexpr size_t BUFFER_SIZE = 4096;
const std::string AUTH_MESSAGE = "AUTH EXTERNAL 31303030\r\n";

using utils::throw_on_error;

void send_message(const std::string& msg) {
  size_t total = 0;
  while (total != msg.size()) {
    ssize_t written = ::write(GLOBAL_SOCKET, msg.data() + total, msg.size() - total);
    throw_on_error(written, "Error writing message");
    total += written;
  }
}

std::string get_message() {
  std::string message;
  std::string buffer(BUFFER_SIZE, 0);
  while (true) {
    ssize_t read = ::read(GLOBAL_SOCKET, buffer.data(), buffer.size());
    throw_on_error(read, "Error reading message");
    message += buffer.substr(0, read);
    if (message.size() > 2 && message.substr(message.size() - 2) == "\r\n")
      break;
  }
  return message;
}

struct Header {
  uint32_t header_1;
  uint32_t body_size;
  uint32_t header_2;
  uint32_t header_size;
};

std::string get_message2() {
  Header header;
  ::read(GLOBAL_SOCKET, &header, sizeof(header));
  header.header_size = static_cast<int>(std::ceil(header.header_size / 8.0)) * 8;
  size_t total = header.body_size + header.header_size;
  std::string message(total, 0);
  size_t read = 0;
  do {
    int chunk = ::read(GLOBAL_SOCKET, message.data() + read, message.size() - read);
    throw_on_error(chunk, "Error reading message");
    read += chunk;
  } while (read != message.size());

  return message.substr(header.header_size);
}

int main(int argc, char *argv[])
{
    GLOBAL_SOCKET = ::socket(AF_UNIX, SOCK_STREAM, 0);
    throw_on_error(GLOBAL_SOCKET, "Error creating socket");

    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_NAME.c_str(), sizeof(addr.sun_path) - 1);

    int res = ::connect(GLOBAL_SOCKET, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
    write(GLOBAL_SOCKET, "\0", 1);
    throw_on_error(res, "Error connecting");

    send_message(AUTH_MESSAGE);
    std::cout << "Received " << get_message() << std::endl;
    send_message("BEGIN\r\n");

    send_message(HELLO_MESSAGE);
    std::string reply = get_message2();
    uint32_t name_size = *reinterpret_cast<uint32_t*>(reply.data());
    std::string name = reply.substr(4, name_size);
    std::cout << "Name " << name << std::endl;
    reply = get_message2();
    std::cout << "Name Ac " << reply.size() << " " << reply << std::endl;

    send_message(ADD_MATCH_1_MESSAGE);
    reply = get_message2();
    std::cout << "AddMatch " << reply.size() << " " << reply << std::endl;

    send_message(ADD_MATCH_2_MESSAGE);
    reply = get_message2();
    std::cout << "AddMatch2 " << reply.size() << " " << reply << std::endl;

    send_message(NOTIFICATION_MESSAGE);
    reply = get_message2();
    std::cout << "Reply " << reply.size() << std::endl;
    ::close(GLOBAL_SOCKET);
}
