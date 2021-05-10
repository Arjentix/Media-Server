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

#include <unordered_map>

#include "sock/socket.h"

#include <ostream>
#include <iomanip>

namespace http {

/**
 * @brief Exception, indicating that an error occurred during request parsing
 */
class ParseError : public std::runtime_error {
 public:
  explicit ParseError(std::string_view message);
};


/**
 * @brief Specific hasher for header names to provide case-insensitivity
 */
struct HeaderNameHash {
  std::size_t operator()(std::string header_name) const;
};

/**
 * @brief Specific equal for header names to provide case-insensitivity
 */
struct HeaderNameEqual {
  bool operator()(std::string lhs, std::string rhs) const;
};

using Headers = std::unordered_map<std::string, std::string,
                                   HeaderNameHash, HeaderNameEqual>;

/**
 * @brief Parse header from string
 *
 * @param header_str String with header
 * @return pair with header name and header value
 */
std::pair<std::string, std::string> ParseHeader(const std::string &header_str);

/**
 * @brief Parse method from string. Should be specialized for Method class in
 * BaseRequest instantiation
 *
 * @param method_str String with method
 * @return Parsed method
 */
template <typename Method>
Method ParseMethod(const std::string &method_str);

/**
 * @brief Convert method to string. Should be specialized for Mehtod class in
 * BaseRequest instantiation
 *
 * @param method Method to convert
 * @return String representation of given method
 */
template <typename Method>
std::string MethodToString(Method method);

/**
 * @brief Base http-like request class with template method field
 *
 * @tparam Method Method class or enum, for which a ParseMethod<Method>() and
 * MethodToString<Method>() specializations exists (see above)
 */
template <typename Method, const char protocol_name[]>
struct BaseRequest {
  virtual ~BaseRequest() = default;

  Method method;
  std::string url;
  float version;
  Headers headers;
  std::string body;
};

/**
 * @brief Parse request from string
 * @throws ParseError if some error occurred during parsing
 *
 * @param request_str String with request
 * @return Extracted request
 */
template <typename Method, const char protocol_name[]>
BaseRequest<Method, protocol_name> ParseRequest(const std::string &request_str) {
  using namespace std::string_literals;
  
  BaseRequest<Method, protocol_name> request;
  std::istringstream iss(request_str);

  std::string method_str;
  iss >> method_str;
  request.method = ParseMethod<Method>(method_str);

  iss >> request.url;

  iss.ignore(1);
  std::string protocol;
  std::getline(iss, protocol, '/');
  if (protocol != protocol_name) {
    throw ParseError("Expected "s + protocol_name + " protocol, but got " + protocol);
  }

  iss >> request.version;

  iss.ignore(2, '\n');
  std::string line;
  while (std::getline(iss, line) && line != "\r") {
    request.headers.insert(ParseHeader(std::move(line)));
  }

  request.body = iss.str().substr(iss.tellg());

  return request;
}

std::ostream &operator<<(std::ostream &os, const Headers &headers);

template <typename Method, const char protocol_name[]>
std::ostream &operator<<(std::ostream &os, const BaseRequest<Method, protocol_name> &request) {
  os << MethodToString(request.method) << " " << request.url << " RTSP/"
     << std::fixed << std::setprecision(1) << request.version << "\r\n"
     << request.headers << "\r\n\r\n"
     << request.body;

  return os;
}


/**
 * @brief Extract value of "Content-Length" header
 *
 * @param headers Request headers where header will be searched
 * @return Extracted value if header exists
 * @return 0 in other case
 */
int ExtractContentLength(const Headers &headers);

template <typename Method, const char protocol_name[]>
sock::Socket &operator>>(sock::Socket &socket,
                         BaseRequest<Method, protocol_name> &request) {
  std::string request_str;
  while (request_str.rfind("\r\n\r\n") == std::string::npos) {
    constexpr int kBuffSize = 1024;
    request_str += socket.Read(kBuffSize);
  }

  request = ParseRequest<Method, protocol_name>(request_str);

  const int content_length = ExtractContentLength(request.headers);
  const int diff = content_length - request.body.size();
  if (diff > 0) {
    request.body += socket.Read(diff);
  }

  return socket;
}

} // namespace http
