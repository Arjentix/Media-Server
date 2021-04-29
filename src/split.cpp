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

#include "split.h"

/**
 * @brief Split string by delimiter
 *
 * @param str String to split
 * @param delim Delimiter string
 * @return Vector of substrings split by delimiter
 */
std::vector<std::string> Split(const std::string &str, const std::string &delim) {
  std::vector<std::string> result;
  size_t pos = str.find(delim);
  size_t initial_pos = 0;

  while (pos != std::string::npos) {
    result.push_back(str.substr(initial_pos, pos - initial_pos));
    initial_pos = pos + delim.length();

    pos = str.find(delim, initial_pos);
  }

  result.push_back(
      str.substr(initial_pos,
                 std::min(pos, str.size()) - initial_pos + delim.length()));

  return result;
}

