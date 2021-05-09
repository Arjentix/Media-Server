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

//#include <iostream>
//#include <fstream>

namespace converters {

Mpeg2TsPackager::Mpeg2TsPackager(const int width, const int height,
                                 const int fps, const float chunk_duration):
width_(width),
height_(height),
fps_(fps),
chunk_duration_(chunk_duration),
frames_per_chunk_(fps_ * chunk_duration_),
chunk_frame_counter_(0),
output_context_ptr_(nullptr),
buffer_data_(),
format_context_ptr_(nullptr),
packet_ptr_(nullptr) {
  InitOutputContext();
  InitFormatContext();
  InitVideoStream();

  packet_ptr_ = av_packet_alloc();

  WriteHeader();
}

Mpeg2TsPackager::~Mpeg2TsPackager() noexcept {
  av_packet_free(&packet_ptr_);
  avformat_free_context(format_context_ptr_);
  av_freep(&output_context_ptr_->buffer);
  avio_context_free(&output_context_ptr_);
}

void Mpeg2TsPackager::Receive(const Bytes &data) {
  ++chunk_frame_counter_;

  WriteFrame(data);

  if (chunk_frame_counter_ >= static_cast<int>(frames_per_chunk_)) {
    WriteTrailer();

    // Saving chunk
//    {
//      using namespace std::string_literals;
//      static int chunk_counter = 0;
//
//      ++chunk_counter;
//      const std::string filename = "chunk"s + std::to_string(chunk_counter) +
//                                   ".mpegts";
//      std::ofstream file(filename, std::ios::out | std::ios::binary);
//      const Bytes &data_to_write = buffer_data_.GetData();
//      file.write(reinterpret_cast<const char *>(data_to_write.data()),
//                 data_to_write.size());
//      std::cout << "Chunk saved in " << filename << std::endl;
//    }

    ProvideToAll(buffer_data_.GetData());

    buffer_data_.Clear();
    chunk_frame_counter_ = 0;

    WriteHeader();
  }
}

void Mpeg2TsPackager::InitOutputContext() {
  const int kOutputContextBufferSize = 4096;

  Byte *output_context_buffer_ptr = reinterpret_cast<Byte *>(
      av_malloc(kOutputContextBufferSize));
  output_context_ptr_ = avio_alloc_context(
      output_context_buffer_ptr, kOutputContextBufferSize, 1, &buffer_data_,
      NULL, BufferData::WritePacket, NULL);
  if (output_context_ptr_ == NULL) {
    throw std::runtime_error("Could not create context");
  }
}

void Mpeg2TsPackager::InitFormatContext() {
  avformat_alloc_output_context2(&format_context_ptr_, NULL, "mpegts", NULL);
  if (format_context_ptr_ == NULL) {
    throw std::runtime_error("Could not create MPEG-2 TS output format context");
  }
  format_context_ptr_->pb = output_context_ptr_;
}

void Mpeg2TsPackager::InitVideoStream() {
  AVCodec *video_codec_ptr = avcodec_find_encoder(AV_CODEC_ID_H264);
  AVStream *video_stream_ptr = avformat_new_stream(format_context_ptr_,
                                                   video_codec_ptr);
  if (video_stream_ptr == NULL) {
    throw std::runtime_error("Could not create new output stream");
  }
  video_stream_ptr->id = format_context_ptr_->nb_streams - 1;
  AVCodecParameters *params_ptr = video_stream_ptr->codecpar;
  params_ptr->codec_id = AV_CODEC_ID_H264;
  params_ptr->codec_type = AVMEDIA_TYPE_VIDEO;
  params_ptr->width = width_;
  params_ptr->height = height_;
}

void Mpeg2TsPackager::WriteHeader() {
  if (avformat_write_header(format_context_ptr_, NULL) < 0) {
    throw std::runtime_error("Could not write header");
  }
}

void Mpeg2TsPackager::WriteFrame(const Bytes &data) {
  av_packet_ref(packet_ptr_, reinterpret_cast<AVPacket *>(
      const_cast<Byte *>(data.data())));

  if (av_interleaved_write_frame(format_context_ptr_, packet_ptr_)) {
    throw std::runtime_error("Can't write packet");
  }

  av_packet_unref(packet_ptr_);
}

void Mpeg2TsPackager::WriteTrailer() {
  if (av_write_trailer(format_context_ptr_) < 0) {
    throw std::runtime_error("Could not write trailer");
  }
}


const Bytes &Mpeg2TsPackager::BufferData::GetData() const {
  return data_;
}

void Mpeg2TsPackager::BufferData::Clear() {
  data_.clear();
}

int Mpeg2TsPackager::BufferData::WritePacket(void *opaque, uint8_t *buf,
                                             const int buf_size) {
  BufferData *buffer_data_ptr = reinterpret_cast<BufferData *>(opaque);

  Bytes &data = buffer_data_ptr->data_;
  data.insert(data.end(), buf, buf + buf_size);

  return buf_size;
}

} // namespace converters
