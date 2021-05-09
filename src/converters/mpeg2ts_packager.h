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
  /**
   * @param width Image width
   * @param height Image height
   * @param fps Video fps
   */
  Mpeg2TsPackager(int width, int height, int fps);

  ~Mpeg2TsPackager() noexcept override;

  Mpeg2TsPackager(const Mpeg2TsPackager &) = delete;
  Mpeg2TsPackager &operator=(const Mpeg2TsPackager &) = delete;

  void Receive(const Bytes &data) override;

 private:
  /**
   * @brief Class for ffmpeg avio_alloc_context() API. Used as buffer
   */
  class BufferData {
   public:
    [[nodiscard]] const Bytes &GetData() const;

    /**
     * @brief Write data from buf to opaque
     * @details Function for ffmpeg avio_alloc_context() API
     *
     * @param opaque Pointer to BufferData
     * @param buf Pointer to data
     * @param buf_size Size of data
     * @return Size of writen data
     */
    static int WritePacket(void *opaque, Byte *buf, int buf_size);

   private:
    Bytes data_;
  };

  const int width_; //!< Image width
  const int height_; //!< Image height
  const int fps_; //!< Video fps
  AVIOContext *output_context_ptr_; //!< Output context to write into buffer
  BufferData buffer_data_; //!< Buffer to write data
  //! Format context to pack data into container
  AVFormatContext *format_context_ptr_;
  AVPacket *packet_ptr_; //!< Packet with compressed data

  /**
   * @brief Init output_context_ptr_
   */
  void InitOutputContext();

  /**
   * @brief Init format_context_ptr_
   */
  void InitFormatContext();

  /**
   * @brief Initialize video stream for format_context_ptr_
   */
  void InitVideoStream();
};

} // namespace converters
