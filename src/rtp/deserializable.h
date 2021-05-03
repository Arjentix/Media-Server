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

#include "byte.h"

/**
 * @brief Interface class for object that can be deserialized from bytes
 */
class Deserializable {
 public:
  virtual void Deserialize(const Bytes &bytes) = 0;
};

/**
 * @brief Check if bytes contains at least expected_size bytes
 * @throw std::invalid_argument, if bytes.size() < expected_size
 *
 * @param bytes Collection of bytes
 * @param expected_size Expected size of bytes collection
 */
void ValidateBytesSize(const Bytes &bytes, std::size_t expected_size);

/**
 * @brief Deserialize integer from bytes
 *
 * @param bytes Bytes to deserialize integers from
 * @return Deserialized integer
 */
uint16_t Deserialize16(const Bytes &bytes);
uint32_t Deserialize24(const Bytes &bytes);
uint32_t Deserialize32(const Bytes &bytes);
