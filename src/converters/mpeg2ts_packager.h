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

#include "frame/observer.h"
#include "frame/provider.h"

extern "C" {
#include <libavformat/avformat.h>
}

namespace converters {

/**
 * @brief This class receives video packets and pack them to the MPEG2-TS container
 * @note In fact it should also pack audio, but it is not supported right now
 */
class Mpeg2TsPackager : public frame::Observer, public frame::Provider {
 public:
  Mpeg2TsPackager();

  ~Mpeg2TsPackager() noexcept;

  Mpeg2TsPackager(const Mpeg2TsPackager &) = delete;
  Mpeg2TsPackager &operator=(const Mpeg2TsPackager &) = delete;

  void Receive(const Bytes &data) override;

 private:
  struct BufferData {
    BufferData();

    ~BufferData();

    uint8_t *buf; //!< Buffer
    size_t size; //!< Size of buffer
    uint8_t *ptr; //!< Pointer to the empty data
    size_t room; //!< Size left in the buffer
  };

  AVIOContext *output_context_ptr_;
  BufferData buffer_data_;
  AVFormatContext *format_context_ptr_;
  AVPacket *packet_ptr_;

  static int WritePacket(void *opaque, uint8_t *buf, int buf_size);
};

} // namespace converters
