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
 * @brief This class receives video packets and pack them into the MPEG2-TS chunks
 * @detals It provides data to it's observes then chunk is ready
 * @note In fact it should also pack audio, but it is not supported right now
 */
class Mpeg2TsPackager : public frame::Observer, public frame::Provider {
 public:
  /**
   * @param width Image width
   * @param height Image height
   * @param fps Video fps
   * @param chunk_duration_sec Chunk max duration in seconds
   */
  Mpeg2TsPackager(int width, int height, int fps, float chunk_duration);

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
    /**
     * @brief Get saved data
     *
     * @return Data
     */
    [[nodiscard]] const Bytes &GetData() const;

    /**
     * @brief Clear buffer
     */
    void Clear();

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
  const int chunk_duration_; //!< Chunk max duration
  const float frames_per_chunk_; //!< Number of frames per chunk
  int chunk_frame_counter_; //!< Number of frames for current chunk
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

  /**
   * @brief Write container header
   */
  void WriteHeader();

  /**
   * @brief Write frame to buffer
   *
   * @param data Encoded frame (packet) represented in bytes
   */
  void WriteFrame(const Bytes &data);

  /**
   * @brief Write container trailer
   */
  void WriteTrailer();
};

} // namespace converters
