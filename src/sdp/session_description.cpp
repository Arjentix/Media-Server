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

#include "session_description.h"

#include <sstream>

#include "split.h"

namespace {

std::istream &operator>>(std::istream &is, std::pair<uint64_t, uint64_t> &active_time) {
  is >> active_time.first >> active_time.second;

  return is;
}

/**
 * @brief Read sdp value with given key
 *
 * @param is Input stream
 * @param key Sdp key character
 * @param value Value to read
 */
template <typename T>
void Read(std::istream &is, const char key, T &value) {
  using namespace std::string_literals;

  char read_key;
  is.get(read_key);
  if (read_key != key) {
    throw sdp::ParseError("Expected key \""s + key + "\", but got \"" + read_key + "\"");
  }
  is.ignore(1);
  is >> value;
  is.ignore(256, '\n');
}

/**
 * @brief Read sdp value with given key
 *
 * @param is Input stream
 * @param key Sdp key character
 * @param value String value to read to the end of line
 */
void Read(std::istream &is, const char key, std::string &value) {
  const std::istream::pos_type begin_pos = is.tellg();
  char first_ch;
  Read(is, key, first_ch);
  is.seekg(begin_pos);

  getline(is, value, '=');
  getline(is, value, '\r');
  is.ignore(256, '\n');
}

/**
 * @brief Try to read optional value
 *
 * @param is Input stream
 * @param key Sdp key character
 * @param value Value to read
 */
template <typename T>
void TryToRead(std::istream &is, const char key, T &value) {
  const std::istream::pos_type pos = is.tellg();

  try {
    Read(is, key, value);
  } catch (const sdp::ParseError &error) {
    is.seekg(pos);
  }
}

/**
 * @brief Try to read optional string values
 *
 * @param is Input stream
 * @param key Sdp key character
 * @param value Vector to fill with values
 */
void TryToRead(std::istream &is, const char key, std::vector<std::string> &values) {
  while (is) {
    std::string value;
    TryToRead(is, key, value);
    if (value.empty()) {
      break;
    }
    values.push_back(std::move(value));
  }
}

} // namespace

namespace sdp {

ParseError::ParseError(std::string_view message):
std::runtime_error({message.data(), message.size()}) {
}

template <typename T>
std::istream &operator>>(std::istream &is, std::vector<T> &v) {
  std::istream::pos_type pos = is.tellg();

  try {
    while (is) {
      pos = is.tellg();
      T value;
      is >> value;
      v.push_back(std::move(value));
    }
  } catch (const sdp::ParseError &error) {
    is.seekg(pos);
  }

  return is;
}

std::istream &operator>>(std::istream &is, Attribute &attribute) {
  std::string attribute_str;
  Read(is, 'a', attribute_str);

  const std::vector<std::string> parts = Split(attribute_str, ":");
  attribute.first = parts[0];
  attribute.second = parts[1];

  return is;
}

std::istream &operator>>(std::istream &is, TimeDescription &time_description) {
  Read(is, 't', time_description.active_time);

  std::string repeat_str;
  TryToRead(is, 'r', repeat_str);
  if (repeat_str == "") {
    time_description.repeat = std::nullopt;
  } else {
    time_description.repeat = std::stoi(repeat_str);
  }

  return is;
}

std::istream &operator>>(std::istream &is, MediaDescription &media_description) {
  Read(is, 'm', media_description.name);

  TryToRead(is, 'i', media_description.info);
  TryToRead(is, 'c', media_description.connection);
  TryToRead(is, 'b', media_description.bandwidths);
  TryToRead(is, 'k', media_description.key);
  is >> media_description.attributes;

  return is;
}

SessionDescription ParseSessionDescription(const std::string &str) {
  SessionDescription session_description;
  std::istringstream iss(str);

  Read(iss, 'v', session_description.version);
  Read(iss, 'o', session_description.originator_and_session_id);
  Read(iss, 's', session_description.session_name);

  TryToRead(iss, 'i', session_description.info);
  TryToRead(iss, 'u', session_description.uri);
  TryToRead(iss, 'e', session_description.emails);
  TryToRead(iss, 'p', session_description.phones);
  TryToRead(iss, 'c', session_description.connection);
  TryToRead(iss, 'b', session_description.bandwidths);
  iss >> session_description.time_descriptions;
  TryToRead(iss, 'z', session_description.time_descriptions);
  TryToRead(iss, 'k', session_description.key);
  iss >> session_description.attributes;
  iss >> session_description.media_descriptions;

  return session_description;
}

} // namespace sdp
