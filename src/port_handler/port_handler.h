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

#include <iostream>
#include <vector>
#include <future>

#include "sock/exception.h"
#include "request_dispatcher.h"

namespace port_handler {

/**
 * @brief Class to accept clients on given port and handle theirs requests
 *
 * @tparam RequestType Type of request, that can be read from socket
 * @tparam ResponseType Type of response, that can be sent to socket
 */
template <typename RequestType, typename ResponseType>
class PortHandler : public PortHandlerBase {
 public:

  /**
   * @param port Port to handle clients on
   */
  explicit PortHandler(int port) :
  socket_(sock::Type::kTcp, port),
  request_dispatcher_(),
  futures_() {
  }

  PortHandler(const PortHandler &other) = delete;
  PortHandler operator=(const PortHandler &other) = delete;

  const sock::ServerSocket &GetSocket() const override {
    return socket_;
  }

  void AcceptAndHandleClient() override {
    sock::Socket client = socket_.Accept();

    futures_.push_back(
        std::async(std::launch::async, &PortHandler::HandleClient, this,
                   std::make_unique<sock::Socket>(std::move(client))));
  }

  /**
   * @brief Register servlet on provided url
   *
   * @param url Path which is handled by given servlet
   * @param servlet_ptr Pointer to servlet to handle requests on given url
   */
  void RegisterServlet(const std::string &url,
      std::shared_ptr<Servlet<RequestType, ResponseType>> servlet_ptr) {
    request_dispatcher_.RegisterServlet(url, servlet_ptr);
  }

 private:
  sock::ServerSocket socket_;
  RequestDispatcher<RequestType, ResponseType> request_dispatcher_;
  std::vector<std::future<void>> futures_;

  /**
   * @brief Handle client requests
   *
   * @param client_socket_ptr Unique pointer to socket, associated with client
   */
  void HandleClient(std::unique_ptr<sock::Socket> client_socket_ptr) {
    try {
      for (;;) {
        RequestType request;
        (*client_socket_ptr) >> request;

        ResponseType response = request_dispatcher_.Dispatch(request);
        (*client_socket_ptr) << response;
      }
    } catch (const sock::ReadError &) {
      std::cout << "Client on socket " << client_socket_ptr->GetDescriptor()
                << " disconnected" << std::endl;
    } catch (const sock::SendError &) {
      std::cout << "Client on socket " << client_socket_ptr->GetDescriptor()
                << " disconnected while was waiting for response" << std::endl;
    }
    std::cout << "Socket " << client_socket_ptr->GetDescriptor()
              << " closed" << std::endl;
  }
};

} // namespace port_handler
