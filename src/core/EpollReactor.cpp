#include "EpollReactor.hpp"
#include <stdexcept>
#include <unistd.h>

EpollReactor::EpollReactor() : epfd_(::epoll_create(128)) {
  if (epfd_ < 0)
    throw std::runtime_error("epoll_create failed");
}

EpollReactor::~EpollReactor() {
  if (epfd_ >= 0)
    ::close(epfd_);
}

bool EpollReactor::add(int fd, uint32_t ev) {
  epoll_event e;
  e.events = ev;
  e.data.fd = fd;
  return (::epoll_ctl(epfd_, EPOLL_CTL_ADD, fd, &e) == 0);
}

bool EpollReactor::mod(int fd, uint32_t ev) {
  epoll_event e;
  e.events = ev;
  e.data.fd = fd;
  return (::epoll_ctl(epfd_, EPOLL_CTL_MOD, fd, &e) == 0);
}

bool EpollReactor::del(int fd) {
  return (::epoll_ctl(epfd_, EPOLL_CTL_DEL, fd, 0) == 0);
}

int EpollReactor::wait(epoll_event *out, int max, int timeout_ms) {
  return (::epoll_wait(epfd_, out, max, timeout_ms));
}
