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

#include "mpeg2ts_packager.h"

#include <iostream>
#include <stdexcept>

namespace {

const int kBufferSize = 1024;

const float kOneContainerDurationInSec = 10.0;

struct BufferData {
  uint8_t *buf; //!< Buffer
  size_t size; //!< Size of buffer
  uint8_t *ptr; //!< Pointer to the empty data
  size_t room; //!< Size left in the buffer
};

int WritePacket(void *opaque, uint8_t *buf, int buf_size) {
  BufferData *bd = reinterpret_cast<BufferData *>(opaque);
  while (buf_size > static_cast<int>(bd->room)) {
    int64_t offset = bd->ptr - bd->buf;
    bd->buf = reinterpret_cast<uint8_t *>(av_realloc_f(bd->buf, 2, bd->size));
    if (!bd->buf) {
      return AVERROR(ENOMEM);
    }
    bd->size *= 2;
    bd->ptr = bd->buf + offset;
    bd->room = bd->size - offset;
  }
  printf("write packet pkt_size:%d used_buf_size:%zu buf_size:%zu buf_room:%zu\n", buf_size, bd->ptr-bd->buf, bd->size, bd->room);

  memcpy(bd->ptr, buf, buf_size);
  bd->ptr += buf_size;
  bd->room -= buf_size;

  return buf_size;
}

} // namespace

namespace converters {

Mpeg2TsPackager::Mpeg2TsPackager():
format_context_ptr_(nullptr),
buffer_ptr_(nullptr),
output_context_ptr_(nullptr),
packet_ptr_(nullptr) {
//  BufferData bd;

//  buffer_ptr_ = reinterpret_cast<Byte *>(av_malloc(kBufferSize));
//  output_context_ptr_ = avio_alloc_context(buffer_ptr_, kBufferSize, 1, &bd, NULL, WritePacket, NULL);
//  if (output_context_ptr_ == NULL) {
//    throw std::runtime_error("Could not create context");
//  }

//  avformat_alloc_output_context2(&format_context_ptr_, NULL, "mpeg2ts", NULL);
  avformat_alloc_output_context2(&format_context_ptr_, NULL, "mpeg", "video.mpeg"); // Delete
  if (format_context_ptr_ == NULL) {
    throw std::runtime_error("Could not create MPEG-2 TS output format context");
  }
//  format_context_ptr_->pb = output_context_ptr_;
  // vvvvv Delete
  if (!(format_context_ptr_->oformat->flags & AVFMT_NOFILE)) {
    if (avio_open(&format_context_ptr_->pb, "video.mpeg", AVIO_FLAG_WRITE) < 0) {
      throw std::runtime_error("Could not open output file");
    }
  }
  // ^^^^ Delete

  AVCodec *video_codec_ptr = avcodec_find_encoder(AV_CODEC_ID_H264);
  AVStream *video_stream_ptr = avformat_new_stream(format_context_ptr_, video_codec_ptr);
  if (video_stream_ptr == NULL) {
    throw std::runtime_error("Could not create new output stream");
  }
  video_stream_ptr->id = format_context_ptr_->nb_streams - 1;
  AVCodecParameters *params_ptr = video_stream_ptr->codecpar;
  params_ptr->codec_id = AV_CODEC_ID_H264;
  params_ptr->codec_type = AVMEDIA_TYPE_VIDEO;
  params_ptr->width = 1280; //!< @TODO Get width from server
  params_ptr->height = 960; //!< @TODO Get height from server
//  params_ptr->bit_rate = 400'000;

  if (avformat_write_header(format_context_ptr_, NULL) < 0) {
    throw std::runtime_error("Could not write header");
  }

  packet_ptr_ = av_packet_alloc();
}

Mpeg2TsPackager::~Mpeg2TsPackager() noexcept {
  av_write_trailer(format_context_ptr_);

  avio_closep(&format_context_ptr_->pb); // Delete
  av_packet_free(&packet_ptr_);
  avio_context_free(&output_context_ptr_);
  avformat_free_context(format_context_ptr_);
//  av_freep(&buffer_ptr_);
}

void Mpeg2TsPackager::Receive(const Bytes &data) {
  av_packet_ref(packet_ptr_, reinterpret_cast<AVPacket *>(const_cast<Byte *>(data.data())));
  std::cout << "Received pts = " << packet_ptr_->pts << std::endl;

  if (av_interleaved_write_frame(format_context_ptr_, packet_ptr_)) {
    throw std::runtime_error("Can't write packet");
  }

  av_packet_unref(packet_ptr_);
}

} // namespace converters
