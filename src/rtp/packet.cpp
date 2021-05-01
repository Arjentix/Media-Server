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

#include <algorithm>

namespace rtp {

void Packet::Deserialize(const Bytes &bytes) {
  header.version = (bytes[0] & 0xC0) >> 6;
  header.padding = (bytes[0] & 0x20) >> 5;
  header.extension = (bytes[0] & 0x10) >> 4;
  header.csrc_count = (bytes[0] & 0xF);
  header.marker = (bytes[1] & 0x80) >> 7;
  header.payload_type = (bytes[1] & 0x7F);
  header.sequence_number = Deserialize16({bytes.begin() + 2, bytes.begin() + 4});
  header.timestamp = Deserialize32({bytes.begin() + 4, bytes.begin() + 8});
  header.synchronization_source = Deserialize32({bytes.begin() + 8,
                                                 bytes.begin() + 12});
  const auto contributing_sources_begin_it = bytes.begin() + 12;
  uint32_t i = 0;
  for (;
      (i < header.csrc_count) && (i < Header::kContributingSourcesMaxCount);
      ++i) {
    const auto current_begin = contributing_sources_begin_it + 4 * i;
    header.contributing_sources[i] = Deserialize32({current_begin,
                                                    current_begin + 4});
  }
  auto payload_begin_it = contributing_sources_begin_it + 4 * i + 4;
  if (header.extension == 1) {
    const auto extension_header_begin_it = payload_begin_it;
    header.extension_header.id = Deserialize16({extension_header_begin_it,
                                                extension_header_begin_it + 2});
    header.extension_header.length = Deserialize16(
        {extension_header_begin_it + 2, extension_header_begin_it + 4});
    const auto extension_content_begin_it = extension_header_begin_it + 4;
    payload_begin_it = extension_content_begin_it + header.extension_header.length;
    std::copy(extension_content_begin_it,
              payload_begin_it,
              std::back_inserter(header.extension_header.content));
  }
  std::copy(payload_begin_it, bytes.end(), std::back_inserter(payload));
}

sock::Socket &operator>>(sock::Socket &socket, Packet &packet) {
  std::string packet_str = socket.Read(1024);
  Bytes bytes(packet_str.size());
//  std::move(packet_str.begin(), packet_str.end(), bytes.begin());
  for (int i = 0; i < packet_str.size(); ++i) {
    bytes[i] = std::move(packet_str[i]);
  }

  packet.Deserialize(bytes);
  return socket;
}

} // namespace rtp
