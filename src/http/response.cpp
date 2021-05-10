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

#include "response.h"

#include <utility>
#include <iomanip>

namespace {

/**
 * @brief Parse RTSP response from string
 * @throws rtsp::ParseError if some error occurred during parsing
 *
 * @param request_str String with response
 * @return Extracted response
 */
http::Response ParseResponse(std::string &&response_str) {
  http::Response response;

  std::istringstream iss(response_str);

  std::string protocol;
  std::getline(iss, protocol, '/');
  if (protocol != "RTSP") {
    throw http::ParseError("Expected RTSP protocol, but got " + protocol);
  }

  iss >> response.version;
  iss >> response.code;
  iss >> response.description;

  iss.ignore(2, '\n');
  std::string line;
  while (std::getline(iss, line) && line != "\r") {
    response.headers.insert(http::ParseHeader(line));
  }

  response.body = iss.str().substr(iss.tellg());

  return response;
}

} // namespace

namespace http {

Response::Response() :
protocol_name("HTTP"),
version(1.0),
code(0),
description(),
headers(),
body() {
}

Response::Response(int code, std::string description,
                   Headers headers, std::string body) :
protocol_name("HTTP"),
version(1.0),
code(code),
description(std::move(description)),
headers(std::move(headers)),
body(std::move(body)) {
}

std::ostream &operator<<(std::ostream &os, const Response &response) {
  os << response.protocol_name << "/" << std::fixed << std::setprecision(1)
     << response.version << " " << response.code << " " << response.description
     << "\r\n" << response.headers << "\r\n" << response.body;

  return os;
}

sock::Socket &operator>>(sock::Socket &socket, Response &response) {
  std::string response_str;
  while (response_str.rfind("\r\n\r\n") == std::string::npos) {
    constexpr int kBuffSize = 1024;
    response_str += socket.Read(kBuffSize);
  }

  response = ParseResponse(std::move(response_str));

  const int content_length = ExtractContentLength(response.headers);
  const int diff = content_length - response.body.size();
  if (diff > 0) {
    response.body += socket.Read(diff);
  }

  return socket;
}

} // namespace http
