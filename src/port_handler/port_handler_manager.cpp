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

#include "port_handler_manager.h"

namespace port_handler {

PortHandlerManager::PortHandlerManager() :
handlers_(),
cached_fds_ptr_(nullptr),
cached_fds_count_(0) {
}

void PortHandlerManager::RegisterPortHandler(
    port_handler::PortHandlerManager::PortHandlerBasePtr handler_ptr) {
  handlers_.push_back(std::move(handler_ptr));
}

void PortHandlerManager::TryAcceptClients(const int timeout_ms) {
  if (cached_fds_count_ != handlers_.size()) {
    FillFds();
  }

  int res = poll(cached_fds_ptr_.get(), cached_fds_count_, timeout_ms);
  if (res <= 0) {
    return;
  }

  for (std::size_t i = 0; i < cached_fds_count_; ++i) {
    if (cached_fds_ptr_[i].revents & POLLIN) {
      cached_fds_ptr_[i].revents = 0;
      handlers_[i]->AcceptAndHandleClient();
    }
  }
}

void PortHandlerManager::FillFds() {
  cached_fds_count_ = handlers_.size();
  cached_fds_ptr_ = std::make_unique<pollfd[]>(cached_fds_count_);
  for (std::size_t i = 0; i < cached_fds_count_; ++i) {
    cached_fds_ptr_[i].fd = handlers_[i]->GetSocket().GetDescriptor();
    cached_fds_ptr_[i].events = POLLIN;
  }
}

} // namespace port_handler
