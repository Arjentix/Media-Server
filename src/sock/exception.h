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

#include <stdexcept>
#include <string_view>

namespace sock {

/**
 * @brief Base class for Socket exceptions
 */
class SocketException : public std::runtime_error {
 public:
  SocketException(std::string_view message);
};

class ReadError : public SocketException {
 public:
  ReadError(std::string_view message);
};

class SendError : public SocketException {
 public:
  SendError(std::string_view message);
};

/**
 * @brief Base class for ServerSocket exceptions
 */
class ServerSocketException : public SocketException {
 public:
  ServerSocketException(std::string_view message);
};

class BindError : public ServerSocketException {
 public:
  BindError(std::string_view message);
};

class ListenError : public ServerSocketException {
 public:
  ListenError(std::string_view message);
};

class AcceptError : public ServerSocketException {
 public:
  AcceptError(std::string_view message);
};

} // namespace sock
