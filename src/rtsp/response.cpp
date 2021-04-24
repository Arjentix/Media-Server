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
 * @brief Parse RTSP header from string
 *
 * @param header_str String with header
 * @return pair with header name and header value
 */
std::pair<std::string, std::string> ParseHeader(std::string &&header_str) {
  std::istringstream header_iss(header_str);
  std::string header_name;
  std::string header_value;

  std::getline(header_iss, header_name, ':');
  header_iss.ignore(1);
  std::getline(header_iss, header_value, '\r');

  return {header_name, header_value};
}

/**
 * @brief Parse RTSP response from string
 * @throws rtsp::ParseError if some error occurred during parsing
 *
 * @param request_str String with response
 * @return Extracted response
 */
rtsp::Response ParseResponse(std::string &&response_str) {
  rtsp::Response response;

  std::istringstream iss(response_str);

  std::string protocol;
  std::getline(iss, protocol, '/');
  if (protocol != "RTSP") {
    throw rtsp::ParseError("Expected RTSP protocol, but got " + protocol);
  }

  iss >> response.version;
  iss >> response.code;
  iss >> response.description;

  iss.ignore(2, '\n');
  std::string line;
  while (std::getline(iss, line) && line != "\r") {
    response.headers.insert(ParseHeader(std::move(line)));
  }

  response.body = iss.str().substr(iss.tellg());

  return response;
}

/**
 * @brief Extract value of "Content-Length" header
 *
 * @param response Response where header will be searched
 * @return Extracted value if header exists
 * @return 0 in other case
 */
int ExtractContentLength(const rtsp::Response &response) {
  int content_length = 0;

  const std::string kContentLengthHeader = "Content-Length";
  if (response.headers.count(kContentLengthHeader)) {
    content_length = std::stoi(response.headers.at(kContentLengthHeader));
  }

  return content_length;
}

} // namespace

namespace rtsp {

ParseError::ParseError(std::string_view message):
std::runtime_error({message.data(), message.size()}) {
}

Response::Response() :
version(1.0),
code(0),
description(),
headers(),
body() {
}

Response::Response(int code, std::string description,
                   Headers headers, std::string body) :
version(1.0),
code(code),
description(std::move(description)),
headers(std::move(headers)),
body(std::move(body)) {
}

std::ostream &operator<<(std::ostream &os, const Response &response) {
  os << "RTSP/" << std::fixed << std::setprecision(1) << response.version << " "
     << response.code << " " << response.description << "\r\n"
     << response.headers << "\r\n"
     << response.body;

  return os;
}

sock::Socket &operator>>(sock::Socket &socket, Response &response) {
  std::string response_str;
  while (response_str.rfind("\r\n\r\n") == std::string::npos) {
    constexpr int kBuffSize = 1024;
    response_str += socket.Read(kBuffSize);
  }

  response = ParseResponse(std::move(response_str));

  const int content_length = ExtractContentLength(response);
  const int diff = content_length - response.body.size();
  if (diff > 0) {
    response.body += socket.Read(diff);
  }

  return socket;
}

} // namespace rtsp
