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

#include "sock/server_socket.h"

namespace port_handler {

/**
 * @brief Interface class to store PortHandler<Req, Resp> in PortHandlerManager
 */
class PortHandlerBase {
 public:
  virtual ~PortHandlerBase() = default;

  /**
   * @brief Get socket object
   *
   * @return ServerSocket, which is ready to accept clients
   */
  [[nodiscard]] virtual const sock::ServerSocket &GetSocket() const = 0;

  /**
   * @brief Accept client on socket from GetSocket() and handle it in separate thread
   */
  virtual void AcceptAndHandleClient() = 0;
};

} // namespace port_handler
