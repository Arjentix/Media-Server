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

#pragma once

#include "port_handler_base.h"

#include <poll.h>

#include <vector>
#include <memory>

namespace port_handler {

/**
 * @brief Class, that can accept clients on several port handlers
 */
class PortHandlerManager {
 public:
  using PortHandlerBasePtr = std::unique_ptr<PortHandlerBase>;

  PortHandlerManager();

  /**
   * @brief Register port handler
   *
   * @param handler_ptr Pointer to PortHandlerBase inheritor
   */
  void RegisterPortHandler(PortHandlerBasePtr handler_ptr);

  /**
   * @brief Try to accept clients on any of port handlers and handle it in separate thread
   * @details Calls poll() system call
   *
   * @param timeout_ms Timeout to try
   */
  void TryAcceptClients(int timeout_ms);

 private:
  std::vector<PortHandlerBasePtr> handlers_; //!< Collection of registered handlers
  std::unique_ptr<pollfd[]> cached_fds_ptr_; //!< Cached fds for poll()
  std::size_t cached_fds_count_; //!< Size of cached fds

  /**
   * @brief Fill cached fds with handlers sockets
   */
  void FillFds();
};

} // namespace port_handler
