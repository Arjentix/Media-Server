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
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}

namespace converters {

/**
 * @brief Converts MJPEG-encoded video to H264-encoded
 */
 class MjpegToH264 : public frame::Observer, public frame::Provider {
  public:
   /**
    * @param provider Provider of MJPEG-encoded frames
    */
    MjpegToH264();

   ~MjpegToH264() noexcept override;

    MjpegToH264(const MjpegToH264 &) = delete;
    MjpegToH264 &operator=(const MjpegToH264 &) = delete;

    void Receive(const Bytes &data) override;

  private:
   AVCodecContext *dec_context_ptr_;
   AVCodecContext *enc_context_ptr_;
   AVFrame *src_frame_ptr_;
   AVFrame *dst_frame_ptr_;
   AVPacket *src_packet_ptr_;
   AVPacket *dst_packet_ptr_;
   SwsContext *sws_context_ptr_;
   uint64_t frame_counter_;

   void EncodeToH264();
};

} // namespace converters
