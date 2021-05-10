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

#include "observer.h"
#include "provider.h"
#include "types/mjpeg_frame.h"
#include "types/h264_frame.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}

#include <fstream>

namespace converters {

/**
 * @brief Converts MJPEG-encoded video to H264-encoded
 */
 class MjpegToH264 : public Observer<types::MjpegFrame>,
                     public Provider<types::H264Frame> {
  public:
    /**
     * @param width Image width
     * @param height Image height
     * @param fps Video fps
     */
    MjpegToH264(int width, int height, int fps);

   ~MjpegToH264() noexcept override;

    MjpegToH264(const MjpegToH264 &) = delete;
    MjpegToH264 &operator=(const MjpegToH264 &) = delete;

    void Receive(const types::MjpegFrame &frame) override;

  private:
   const int width_;
   const int height_;
   const int fps_;
   AVCodecContext *dec_context_ptr_;
   AVCodecContext *enc_context_ptr_;
   AVFrame *src_frame_ptr_;
   AVFrame *dst_frame_ptr_;
   AVPacket *src_packet_ptr_;
   AVPacket *dst_packet_ptr_;
   SwsContext *sws_context_ptr_;
   uint64_t frame_counter_;

   /**
    * @brief Encode raw frame in src_frame_ with H.264 codec and put it to dst_packet_
    */
   void EncodeToH264();
};

} // namespace converters
