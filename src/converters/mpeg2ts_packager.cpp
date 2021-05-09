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

#include <stdexcept>
#include <fstream> // Delete

namespace {

const int kOutputContextBufferSize = 4096;

const float kOneContainerDurationInSec = 10.0;

} // namespace

namespace converters {

Mpeg2TsPackager::Mpeg2TsPackager():
output_context_ptr_(nullptr),
buffer_data_(),
format_context_ptr_(nullptr),
packet_ptr_(nullptr) {
  Byte *output_context_buffer_ptr = reinterpret_cast<Byte *>(av_malloc(kOutputContextBufferSize));
  output_context_ptr_ = avio_alloc_context(output_context_buffer_ptr, kOutputContextBufferSize, 1, &buffer_data_, NULL, BufferData::WritePacket, NULL);
  if (output_context_ptr_ == NULL) {
    throw std::runtime_error("Could not create context");
  }

  avformat_alloc_output_context2(&format_context_ptr_, NULL, "mpegts", NULL);
  if (format_context_ptr_ == NULL) {
    throw std::runtime_error("Could not create MPEG-2 TS output format context");
  }
  format_context_ptr_->pb = output_context_ptr_;

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

  // Delete next block
  {
    std::ofstream file("video.mpegts", std::ios::out | std::ios::binary);
    file.write(reinterpret_cast<const char *>(buffer_data_.GetBufferPtr()), buffer_data_.GetBufferSize());
  }

  av_packet_free(&packet_ptr_);
  avformat_free_context(format_context_ptr_);
  av_freep(&output_context_ptr_->buffer);
  avio_context_free(&output_context_ptr_);
}

void Mpeg2TsPackager::Receive(const Bytes &data) {
  av_packet_ref(packet_ptr_, reinterpret_cast<AVPacket *>(const_cast<Byte *>(data.data())));

  if (av_interleaved_write_frame(format_context_ptr_, packet_ptr_)) {
    throw std::runtime_error("Can't write packet");
  }

  av_packet_unref(packet_ptr_);
}

Mpeg2TsPackager::BufferData::BufferData():
buffer_ptr_(nullptr),
size_(0),
end_ptr_(nullptr),
left_size_(0) {
  buffer_ptr_ = reinterpret_cast<uint8_t *>(av_malloc(kInitBufferSize));
  if (buffer_ptr_ == NULL) {
    throw std::runtime_error("Can't allocate BufferData buffer");
  }

  end_ptr_ = buffer_ptr_;
  left_size_ = kInitBufferSize;
  size_ = kInitBufferSize;
}

Mpeg2TsPackager::BufferData::~BufferData() {
  av_free(buffer_ptr_);
}

uint8_t *Mpeg2TsPackager::BufferData::GetBufferPtr() {
  return buffer_ptr_;
}

size_t Mpeg2TsPackager::BufferData::GetBufferSize() {
  return size_;
}

int Mpeg2TsPackager::BufferData::WritePacket(void *opaque, uint8_t *buf, int buffer_ptr_size) {
  BufferData *bd = reinterpret_cast<BufferData *>(opaque);
  while (buffer_ptr_size > static_cast<int>(bd->left_size_)) {
    int64_t offset = bd->end_ptr_ - bd->buffer_ptr_;
    bd->buffer_ptr_ = reinterpret_cast<uint8_t *>(av_realloc_f(bd->buffer_ptr_, 2, bd->size_));
    if (!bd->buffer_ptr_) {
      return AVERROR(ENOMEM);
    }
    bd->size_ *= 2;
    bd->end_ptr_ = bd->buffer_ptr_ + offset;
    bd->left_size_ = bd->size_ - offset;
  }

  memcpy(bd->end_ptr_, buf, buffer_ptr_size);
  bd->end_ptr_ += buffer_ptr_size;
  bd->left_size_ -= buffer_ptr_size;

  return buffer_ptr_size;
}

} // namespace converters
