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

namespace {

const int kQuantizationTableSize = 64;

/**
 * @brief Table K.1 from JPEG spec
 */
const int kJpegLumaQuantizer[kQuantizationTableSize] = {
    16, 11, 10, 16, 24, 40, 51, 61,
    12, 12, 14, 19, 26, 58, 60, 55,
    14, 13, 16, 24, 40, 57, 69, 56,
    14, 17, 22, 29, 51, 87, 80, 62,
    18, 22, 37, 56, 68, 109, 103, 77,
    24, 35, 55, 64, 81, 104, 113, 92,
    49, 64, 78, 87, 103, 121, 120, 101,
    72, 92, 95, 98, 112, 100, 103, 99
};

/**
 * @brief Table K.2 from JPEG spec
 */
const int kJpegChromaQuantizer[kQuantizationTableSize] = {
    17, 18, 24, 47, 99, 99, 99, 99,
    18, 21, 26, 66, 99, 99, 99, 99,
    24, 26, 56, 99, 99, 99, 99, 99,
    47, 66, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99
};

/**
 * @brief Write luma- and chroma- quantization tables
 * @details Adopted function from RFC 2435 Appendix A
 *
 * @param q Image quality
 * @param lqt Luma quantization table
 * @param cqt Chroma quantization table
 */
void WriteTables(int q, types::Bytes &lqt, types::Bytes &cqt) {
  int i;
  int factor = std::clamp(q, 1, 99);
  if (q < 50) {
    q = 5000 / factor;
  } else {
    q = 200 - factor * 2;
  }

  for (i = 0; i < kQuantizationTableSize; ++i) {
    int lq = (kJpegLumaQuantizer[i] * q + 50) / 100;
    lq = std::clamp(lq, 1, 255);
    lqt[i] = lq;

    int cq = (kJpegChromaQuantizer[i] * q + 50) / 100;
    cq = std::clamp(cq, 1, 255);
    cqt[i] = cq;
  }
}

const types::Bytes kLumDcCodelens = {
    0, 1, 5, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0
};

const types::Bytes kLumDcSymbols = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11
};

const types::Bytes kLumAcCodelens = {
    0, 2, 1, 3, 3, 2, 4, 3, 5, 5, 4, 4, 0, 0, 1, 0x7d
};

const types::Bytes kLumAcSymbols = {
    0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12,
    0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 0x61, 0x07,
    0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xa1, 0x08,
    0x23, 0x42, 0xb1, 0xc1, 0x15, 0x52, 0xd1, 0xf0,
    0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0a, 0x16,
    0x17, 0x18, 0x19, 0x1a, 0x25, 0x26, 0x27, 0x28,
    0x29, 0x2a, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
    0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49,
    0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59,
    0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69,
    0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79,
    0x7a, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
    0x8a, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98,
    0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
    0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6,
    0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3, 0xc4, 0xc5,
    0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2, 0xd3, 0xd4,
    0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xe1, 0xe2,
    0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea,
    0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
    0xf9, 0xfa
};

const types::Bytes kChmDcCodelens = {
    0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0
};

const types::Bytes kChmDcSymbols = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11
};

const types::Bytes kChmAcCodelens = {
    0, 2, 1, 2, 4, 4, 3, 4, 7, 5, 4, 4, 0, 1, 2, 0x77
};

const types::Bytes kChmAcSymbols = {
    0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21,
    0x31, 0x06, 0x12, 0x41, 0x51, 0x07, 0x61, 0x71,
    0x13, 0x22, 0x32, 0x81, 0x08, 0x14, 0x42, 0x91,
    0xa1, 0xb1, 0xc1, 0x09, 0x23, 0x33, 0x52, 0xf0,
    0x15, 0x62, 0x72, 0xd1, 0x0a, 0x16, 0x24, 0x34,
    0xe1, 0x25, 0xf1, 0x17, 0x18, 0x19, 0x1a, 0x26,
    0x27, 0x28, 0x29, 0x2a, 0x35, 0x36, 0x37, 0x38,
    0x39, 0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
    0x49, 0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
    0x59, 0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
    0x69, 0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78,
    0x79, 0x7a, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
    0x88, 0x89, 0x8a, 0x92, 0x93, 0x94, 0x95, 0x96,
    0x97, 0x98, 0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5,
    0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4,
    0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3,
    0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2,
    0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda,
    0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9,
    0xea, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
    0xf9, 0xfa
};

/**
 * @brief Write quantization table
 * @details Adapted function from RFC 2435 Appendix B
 *
 * @param dest JPEG header to write into
 * @param qt Quantization table
 * @param tableNo Number of quantization table
 */
void WriteQuantHeader(types::Bytes &dest, const types::Bytes &qt, const int tableNo) {
  dest.push_back(0xff);
  dest.push_back(0xdb);
  dest.push_back(0);
  dest.push_back(67);
  dest.push_back(tableNo);
  dest.insert(dest.end(), qt.begin(), qt.begin() + 64);
}

/**
 * @brief Write Huffman header
 * @details Adapted function from RFC 2435 Appendix B
 *
 * @param dest JPEG header to write into
 * @param codelens Codelens
 * @param symbols Symbols
 * @param tableNo Number of Huffman table
 * @param tableClass Class of Huffman table
 */
void WriteHuffmanHeader(types::Bytes &dest, const types::Bytes &codelens, const types::Bytes &symbols, const int tableNo, const int tableClass) {
  dest.push_back(0xff);
  dest.push_back(0xc4);
  dest.push_back(0);
  dest.push_back(3 + codelens.size() + symbols.size()); /* length lsb */
  dest.push_back((tableClass << 4) | tableNo);
  dest.insert(dest.end(), codelens.begin(), codelens.end());
  dest.insert(dest.end(), symbols.begin(), symbols.end());
}

/**
 * @brief Write DRI header
 * @details Adapted function from RFC 2435 Appendix B
 *
 * @param dest JPEG header to write into
 * @param dri DRI parameter
 */
void WriteDriHeader(types::Bytes &dest, const u_short dri) {
  dest.push_back(0xff);
  dest.push_back(0xdd);
  dest.push_back(0x0);
  dest.push_back(4);
  dest.push_back(dri >> 8);
  dest.push_back(dri & 0xff);
}

/**
 * @brief Build JPEG headers
 * @details Adapted function from RFC 2435 Appendix B
 *
 * @param type JPEG coding type from MJPEG packet
 * @param w Image width from MJPEG packet
 * @param h Image height from MJPEG packet
 * @param lqt Luma quantization table
 * @param cqt Chroma quantization table
 * @param dri DRI parameter
 * @return JPEG headers
 */
types::Bytes BuildHeaders(const int type, int w, int h, const types::Bytes &lqt, const types::Bytes &cqt, const u_short dri) {
  types::Bytes headers;

  w <<= 3;
  h <<= 3;

  headers.push_back(0xff);
  headers.push_back(0xd8); // SOI

  WriteQuantHeader(headers, lqt, 0);
  WriteQuantHeader(headers, cqt, 1);

  if (dri != 0) {
    WriteDriHeader(headers, dri);
  }

  headers.push_back(0xff);
  headers.push_back(0xc0); // SOF
  headers.push_back(0); // length msb
  headers.push_back(17); // length lsb
  headers.push_back(8); // 8-bit precision
  headers.push_back(h >> 8); // height msb
  headers.push_back(h); // height lsb
  headers.push_back(w >> 8); // width msb
  headers.push_back(w); // width lsb
  headers.push_back(3); // number of components
  headers.push_back(0); // comp 0
  if (type == 0) {
    headers.push_back(0x21); // hsamp = 2, vsamp = 1
  } else {
    headers.push_back(0x22); // hsamp = 2, vsamp = 2
  }
  headers.push_back(0); // quant table 0
  headers.push_back(1); // comp 1
  headers.push_back(0x11); // hsamp = 1, vsamp = 1
  headers.push_back(1); // quant table 1
  headers.push_back(2); // comp 2
  headers.push_back(0x11); // hsamp = 1, vsamp = 1
  headers.push_back(1); // quant table 1
  WriteHuffmanHeader(headers, kLumDcCodelens, kLumDcSymbols, 0, 0);
  WriteHuffmanHeader(headers, kLumAcCodelens, kLumAcSymbols, 0, 1);
  WriteHuffmanHeader(headers, kChmDcCodelens, kChmDcSymbols, 1, 0);
  WriteHuffmanHeader(headers, kChmAcCodelens, kChmAcSymbols, 1, 1);

  headers.push_back(0xff);
  headers.push_back(0xda); // SOS
  headers.push_back(0); // length msb
  headers.push_back(12); // length lsb
  headers.push_back(3); // 3 components
  headers.push_back(0); // comp 0
  headers.push_back(0); // huffman table 0
  headers.push_back(1); // comp 1
  headers.push_back(0x11); // huffman table 1
  headers.push_back(2); // comp 2
  headers.push_back(0x11); // huffman table 1
  headers.push_back(0); // first DCT coeff
  headers.push_back(63); // last DCT coeff
  headers.push_back(0); // successive approx.

  return headers;
}

} // namespace

namespace rtp::mjpeg {

void Packet::Deserialize(const types::Bytes &bytes) {
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

types::Bytes UnpackJpeg(const std::vector<Packet> &packets) {
  types::Bytes luma_quantization_table(kQuantizationTableSize);
  types::Bytes chroma_quantization_table(kQuantizationTableSize);

  const uint8_t quality = packets[0].header.quality;
  if (quality > 127) {
   const types::Bytes &data = packets[0].header.quantization_table_header.data;
   luma_quantization_table.insert(luma_quantization_table.end(), data.begin(),
                                  data.begin() + kQuantizationTableSize);
   chroma_quantization_table.insert(chroma_quantization_table.end(),
                                    data.begin() + kQuantizationTableSize,
                                    data.begin() + 2 * kQuantizationTableSize);
  } else {
    WriteTables(quality, luma_quantization_table, chroma_quantization_table);
  }

  const Header &header = packets[0].header;
  const int kDri = 0;
  types::Bytes jpeg_image = BuildHeaders(header.type, header.width, header.height, luma_quantization_table, chroma_quantization_table, kDri);
  for (auto it = packets.begin(); it != packets.end(); ++it) {
    jpeg_image.insert(jpeg_image.end(), it->payload.begin(), it->payload.end());
  }

  return jpeg_image;
}
} // namespace rtp::mjpeg
