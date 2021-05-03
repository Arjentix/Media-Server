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

#include "packet.h"

namespace rtp::mjpeg {

void Packet::Deserialize(const Bytes &bytes) {
  ValidateBytesSize(bytes, 8);

  header.type_specific = bytes[0];
  header.fragment_offset = Deserialize24({bytes.begin() + 1, bytes.begin() + 4});
  header.type = bytes[4];
  header.quality = bytes[5];
  header.width = bytes[6];
  header.height = bytes[7];
  auto payload_begin_it = bytes.begin() + 8;
  if (header.type >= 64 && header.type < 128) {
    payload_begin_it += 4;
    header.restart_marker_header = Deserialize32({bytes.begin() + 8, payload_begin_it});
  }
  if (header.quality >= 128) {
    auto it = payload_begin_it;
    header.quantization_table_header.mbz = *(it++);
    header.quantization_table_header.precision = *(it++);
    header.quantization_table_header.length = *(it++);
    header.quantization_table_header.data.insert(
        header.quantization_table_header.data.end(),
        it,
        it + header.quantization_table_header.length);
    payload_begin_it += 3 + header.quantization_table_header.length;
  }
  payload.insert(payload.end(), payload_begin_it, bytes.end());
}

} // namespace rtp::mjpeg
