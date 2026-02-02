#pragma once
#include <netinet/in.h>

class Listener {
  mutable int fd_;

public:
  Listener();
  ~Listener();
  Listener(const Listener &other);
  Listener &operator=(const Listener &other);
  bool operator==(const int other) const;
  bool bindAndListen(uint32_t ip_be, uint16_t port_be, int backlog = 128);
  int fd() const { return (fd_); }
};

int set_nonblock(int fd);