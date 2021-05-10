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

#include "base_request.h"

#include <algorithm>

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

namespace http {

ParseError::ParseError(std::string_view message) :
    std::runtime_error({message.data(), message.size()}) {
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

std::pair<std::string, std::string> ParseHeader(const std::string &header_str) {
  std::istringstream header_iss(header_str);
  std::string header_name;
  std::string header_value;

  std::getline(header_iss, header_name, ':');
  header_iss.ignore(1);
  getline(header_iss, header_value, '\r');
  header_iss.ignore(1, '\n');

  return {header_name, header_value};
}

int ExtractContentLength(const Headers &headers) {
  int content_length = 0;

  const std::string kContentLengthHeader = "Content-Length";
  if (headers.count(kContentLengthHeader)) {
  content_length = stoi(headers.at(kContentLengthHeader));
  }

  return content_length;
}

std::ostream &operator<<(std::ostream &os, const Headers &headers) {
  for (const auto &[key, value] : headers) {
    os << key << ": " << value << "\r\n";
  }

  return os;
}

} // namespace http
