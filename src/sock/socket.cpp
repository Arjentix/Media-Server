/*
MIT License

Copyright (c) 2021 Polyakov Daniil Alexandrovich

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "socket.h"

#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <memory>

#include "exception.h"

namespace sock {

Socket::Socket(Type type):
descriptor_(0),
type_(type),
is_moved_(false) {
  int real_type = 0;
  switch (type) {
    case Type::kTcp:
      real_type = SOCK_STREAM;
      break;
    case Type::kUdp:
      real_type = SOCK_DGRAM;
      break;
    default:
      real_type = 0;
  }

  descriptor_ = socket(AF_INET, real_type, 0);
  if (descriptor_ == -1) {
    throw SocketException("Can't create socket");
  }
}

Socket::Socket(int descriptor) :
descriptor_(descriptor),
is_moved_(false) {
  int type;
  socklen_t length = sizeof(type);
  getsockopt(descriptor_, SOL_SOCKET, SO_TYPE, &type, &length);
  type_ = (type == SOCK_STREAM ? Type::kTcp : Type::kUdp);
}

Socket::Socket(Socket &&other) :
descriptor_(other.descriptor_),
type_(other.type_),
is_moved_(false) {
  other.is_moved_ = true;
}

Socket::~Socket() {
  if (!is_moved_) {
    close(descriptor_);
  }
}

int Socket::GetDescriptor() const {
  return descriptor_;
}

Type Socket::GetType() const {
  return type_;
}

std::string Socket::GetPeerName() const {
  sockaddr_in peer_addr;
  int peer_len = sizeof(peer_addr);
  int res = getpeername(descriptor_, reinterpret_cast<sockaddr *>(&peer_addr),
                        reinterpret_cast<socklen_t *>(&peer_len));
  if (res != 0) {
    return "";
  }

  return inet_ntoa(peer_addr.sin_addr);
}

std::string Socket::Read(int n) {
  auto buf_ptr = std::make_unique<char[]>(n);
  sockaddr_in their_addr;
  socklen_t addr_len = sizeof(their_addr);
  const int flags = (type_ == Type::kTcp ? 0 : MSG_WAITALL);

  int res = recvfrom(descriptor_, buf_ptr.get(), n, flags,
                     reinterpret_cast<sockaddr *>(&their_addr), &addr_len);
  if (res < 0) {
    throw ReadError(strerror(errno));
  }

  if (type_ == Type::kTcp && res == 0 && n != 0) {
    throw ReadError("Socket is closed");
  }

  return {buf_ptr.get(), buf_ptr.get() + res};
}


void Socket::Send(std::string_view str) {
  if (send(descriptor_, str.data(), str.length(), 0) < 0) {
    throw SendError(strerror(errno));
  }
}

void Socket::SendTo(const types::Bytes &bytes, const std::string &ip, int port) {
  sockaddr_in their_addr;
  their_addr.sin_family = AF_INET;
  their_addr.sin_port = htons(port);
  if (inet_pton(AF_INET, ip.c_str(), &their_addr.sin_addr) != 1) {
    throw std::invalid_argument("Invalid ip address");
  }

  int res = sendto(descriptor_, bytes.data(), bytes.size(), MSG_CONFIRM,
                   reinterpret_cast<sockaddr *>(&their_addr), sizeof(their_addr));
  if (res < 0) {
    throw SendError(strerror(errno));
  }
}

Socket &Socket::operator=(Socket &&other) {
  descriptor_ = other.descriptor_;
  type_ = other.type_;
  other.is_moved_ = true;

  return *this;
}

Socket &operator<<(Socket &socket, std::ostream &(*f)(std::ostream &)) {
  if (f == &(std::endl<char, std::char_traits<char>>)) {
    socket.Send(socket.ss_buffer_.str());
    socket.ss_buffer_.str("");
    socket.ss_buffer_.clear();
  } else {
    f(socket.ss_buffer_);
  }

  return socket;
}

} // namespace sock
