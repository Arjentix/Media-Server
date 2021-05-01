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

#include "deserializable.h"

#include <string>
#include <stdexcept>

namespace {

/**
 * @brief Check if bytes contains at least expected_size bytes
 * @throw std::invalid_argument, if bytes.size() < expected_size
 *
 * @param bytes Collection of bytes
 * @param expected_size Expected size of bytes collection
 */
void ValidateBytesSize(const Bytes &bytes, std::size_t expected_size) {
  using namespace std::string_literals;

  if (bytes.size() < expected_size) {
    throw std::invalid_argument("Expected at least "s + std::to_string(2) + " bytes");
  }
}

} // namespace

uint16_t Deserialize16(const Bytes &bytes) {
  ValidateBytesSize(bytes, 2);

  return ((uint16_t(bytes[0]) << 8) | uint16_t(bytes[1]));
}

uint32_t Deserialize32(const Bytes &bytes) {
  ValidateBytesSize(bytes, 4);

  return ((uint32_t(bytes[0]) << 24) | (uint32_t(bytes[1]) << 16) |
          (uint32_t(bytes[2]) << 8) | uint32_t(bytes[3]));
}
