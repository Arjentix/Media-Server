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

#include "request.h"

#include <cctype>

#include <utility>
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace {

/**
 * @brief Transform string to lower case
 *
 * @param str String to be transformed
 */
void TransformToLowerCase(std::string &str) {
  std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) {
    return std::tolower(c);
  });
}

} // namespace

namespace rtsp {

std::string MethodToString(Method method) {
  std::string method_str;

  switch (method) {
    case Method::kDescribe:
      method_str = "DESCRIBE";
      break;
    case Method::kAnnounce:
      method_str = "ANNOUNCE";
      break;
    case Method::kGetParameter:
      method_str = "GET_PARAMETER";
      break;
    case Method::kOptions:
      method_str = "OPTIONS";
      break;
    case Method::kPause:
      method_str = "PAUSE";
      break;
    case Method::kPlay:
      method_str = "PLAY";
      break;
    case Method::kRecord:
      method_str = "RECORD";
      break;
    case Method::kSetup:
      method_str = "SETUP";
      break;
    case Method::kSetParameter:
      method_str = "SET_PARAMETER";
      break;
    case Method::kTeardown:
      method_str = "TEARDOWN";
      break;
    default:
      method_str = "UNKNOWN METHOD";
  }

  return method_str;
}

std::size_t HeaderNameHash::operator()(std::string header_name) const {
  TransformToLowerCase(header_name);
  return std::hash<std::string>()(header_name);
}

bool HeaderNameEqual::operator()(std::string lhs, std::string rhs) const {
  TransformToLowerCase(lhs);
  TransformToLowerCase(rhs);
  return (lhs == rhs);
}

std::ostream &operator<<(std::ostream &os, const Request::Headers &headers) {
  for (const auto &[key, value] : headers) {
    os << key << ": " << value << "\r\n";
  }

  return os;
}

std::ostream &operator<<(std::ostream &os, const Request &request) {
  os << MethodToString(request.method) << " " << request.url << " RTSP/"
     << std::fixed << std::setprecision(1) << request.version << "\r\n"
     << request.headers << "\r\n\r\n"
     << request.body;

  return os;
}

} // namespace rtsp
