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

#include "mjpeg_to_h264.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
}

#include <iostream>
#include <string>

namespace converters {

MjpegToH264::MjpegToH264():
dec_context_ptr_(nullptr),
enc_context_ptr_(nullptr),
src_frame_ptr_(nullptr),
dst_frame_ptr_(nullptr),
src_packet_ptr_(nullptr),
dst_packet_ptr_(nullptr),
dst_size_(0),
sws_context_ptr_(0),
file_("video.h264", std::ios::out | std::ios::binary) {
  AVCodec *dec = avcodec_find_decoder(AV_CODEC_ID_MJPEG);
  dec_context_ptr_ = avcodec_alloc_context3(dec);
  dec_context_ptr_->width = 1280; //!< @TODO Get width from server
  dec_context_ptr_->height = 960; //!< @TODO Get height from server
  dec_context_ptr_->pix_fmt = AV_PIX_FMT_YUV420P;

  // Not full frame on output?
  if (dec->capabilities & AV_CODEC_CAP_TRUNCATED) {
    dec_context_ptr_->flags |= AV_CODEC_FLAG_TRUNCATED;
  }

  if (avcodec_open2(dec_context_ptr_, dec, NULL) < 0) {
    throw std::runtime_error("avcodec_open2 error with decoding context");
  }

  AVCodec *enc = avcodec_find_encoder(AV_CODEC_ID_H264);
  enc_context_ptr_ = avcodec_alloc_context3(enc);
  enc_context_ptr_->width = 1280; //!< @TODO Get width from server
  enc_context_ptr_->height = 960; //!< @TODO Get height from server
  enc_context_ptr_->bit_rate = 1024;
  enc_context_ptr_->pix_fmt = AV_PIX_FMT_YUV420P;
  enc_context_ptr_->time_base.den = 10; //!< @TODO Get fps from server
  enc_context_ptr_->time_base.num = 1;
  enc_context_ptr_->gop_size = 12;
  enc_context_ptr_->max_b_frames = 0;
  av_opt_set(enc_context_ptr_->priv_data, "preset", "slow", 0);

  if (avcodec_open2(enc_context_ptr_, enc, NULL) < 0) {
    throw std::runtime_error("avcodec_open2 error with encoding context");
  }

  sws_context_ptr_ = sws_getContext(
      dec_context_ptr_->width, dec_context_ptr_->height, dec_context_ptr_->pix_fmt,
      enc_context_ptr_->width, enc_context_ptr_->height, enc_context_ptr_->pix_fmt,
      SWS_BILINEAR, NULL, NULL, NULL);

  src_frame_ptr_ = av_frame_alloc();

  src_packet_ptr_ = av_packet_alloc();

  dst_frame_ptr_ = av_frame_alloc();
  dst_size_ = av_image_alloc(dst_frame_ptr_->data, dst_frame_ptr_->linesize, enc_context_ptr_->width, enc_context_ptr_->height, enc_context_ptr_->pix_fmt, 32);
  dst_frame_ptr_->width = enc_context_ptr_->width;
  dst_frame_ptr_->height = enc_context_ptr_->height;
  dst_frame_ptr_->format = static_cast<int>(enc_context_ptr_->pix_fmt);
  dst_frame_ptr_->pts = 0;

  dst_packet_ptr_ = av_packet_alloc();
}

MjpegToH264::~MjpegToH264() noexcept {
  avcodec_free_context(&dec_context_ptr_);
  avcodec_free_context(&enc_context_ptr_);
  av_freep(&dst_frame_ptr_->data);
  av_frame_free(&src_frame_ptr_);
  av_frame_free(&dst_frame_ptr_);
  av_packet_free(&src_packet_ptr_);
  av_packet_free(&dst_packet_ptr_);
}

static int frame_counter = 0;

static void PgmSave(unsigned char *buf, int wrap, int xsize, int ysize,
                    char *filename)
{
  FILE *f;
  int i;

  f = fopen(filename,"wb");
  fprintf(f, "P5\n%d %d\n%d\n", xsize, ysize, 255);
  for (i = 0; i < ysize; i++)
    fwrite(buf + i * wrap, 1, xsize, f);
  fclose(f);
}

void MjpegToH264::Receive(const Bytes &data) {
  ++frame_counter;
  std::cout << "Received MJPEG frame of " << data.size() << " bytes size" << std::endl;

  src_packet_ptr_->data = const_cast<Byte *>(data.data());
  src_packet_ptr_->size = data.size();

  int res = avcodec_send_packet(dec_context_ptr_, src_packet_ptr_);
  if (res < 0) {
    throw std::runtime_error("Error sending packet for decoding");
  }

  while (res >= 0) {
    res = avcodec_receive_frame(dec_context_ptr_, dst_frame_ptr_);
    if ((res == AVERROR(EAGAIN)) || (res == AVERROR_EOF)) {
      return;
    } else if (res < 0) {
      throw std::runtime_error("Error during decoding");
    }

    EncodeToH264();
  }
}

void MjpegToH264::EncodeToH264() {
  sws_scale(sws_context_ptr_, src_frame_ptr_->data, src_frame_ptr_->linesize, 0,
            src_frame_ptr_->height, dst_frame_ptr_->data, dst_frame_ptr_->linesize);

  dst_frame_ptr_->pts = (1.0 / 30) * 90 * frame_counter;
  dst_packet_ptr_->pts = dst_frame_ptr_->pts;
  dst_packet_ptr_->dts = dst_packet_ptr_->pts;

//  char buf[1024];
//  snprintf(buf, sizeof(buf), "frame-%d.pgm", dec_context_ptr_->frame_number);
//  PgmSave(dst_frame_ptr_->data[0], dst_frame_ptr_->linesize[0],
//          dst_frame_ptr_->width, dst_frame_ptr_->height, buf);
//  std::cout << "Saved " << buf << std::endl;

  int res = avcodec_send_frame(enc_context_ptr_, dst_frame_ptr_);
  if (res < 0) {
    throw std::runtime_error("Error sending frame for encoding");
  }

  while (res >= 0) {
    res = avcodec_receive_packet(enc_context_ptr_, dst_packet_ptr_);
    if ((res == AVERROR(EAGAIN)) || (res == AVERROR_EOF)) {
      return;
    } else if (res < 0) {
      throw std::runtime_error("Error during encoding");
    }

    std::cout << "Prepared H264 frame of " << dst_packet_ptr_->size << " bytes size" << std::endl;
    ProvideToAll({dst_packet_ptr_->data, dst_packet_ptr_->data + dst_packet_ptr_->size});
    char const start_code[4] = {0x00, 0x00, 0x00, 0x01};
    file_.write(start_code, 4);
    file_.write(reinterpret_cast<const char *>(dst_packet_ptr_->data),
                dst_packet_ptr_->size);
    av_packet_unref(dst_packet_ptr_);
  }
}

} // namespace converters
