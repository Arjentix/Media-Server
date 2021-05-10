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

extern "C" {
#include <libavformat/avformat.h>
}

#include <iostream>
#include <algorithm>
#include <regex>
#include <sstream>

#include "servlet.h"
#include "http/request.h"
#include "http/response.h"
#include "frame/observer.h"

namespace hls {

const char kContentLengthHeaderName[] = "Content-Length";
const char kPlaylistPath[] = "/playlist.m3u";
const std::regex kChunkPathRegex("^/chunk(\\d+)\\.ts$");
const http::Response NotFoundResponse = {404, "Not Found"};

class Servlet : public ::Servlet<http::Request, http::Response>,
                public frame::Observer {
 public:
  /**
   * @param chunk_count Number of chunk to store in memory
   * @param chunk_duration Max duration of one chunk in seconds
   */
  Servlet(int chunk_count, float chunk_duration):
  chunks_(chunk_count),
  chunk_duration_(chunk_duration) {
  }

  [[nodiscard]] http::Response Handle(const http::Request &request) override {
    if (request.method == http::Method::kGet) {
      return HandleGet(request);
    }

    return {501, "Not Implemented"};
  }

  /**
   * @param data MPEG2-TS data represented in bytes
   */
  void Receive(const Bytes &data) override {
    Chunk &last_chunk = chunks_.back();
    const uint64_t last_chunk_number = last_chunk.media_sequence_number;

    for (std::size_t i = 0; i < chunks_.size() - 1; ++i) {
      chunks_[i] = std::move(chunks_[i + 1]);
    }

    last_chunk.media_sequence_number = last_chunk_number + 1;
    last_chunk.duration = 0; //!< @TODO Normal duration extracting
    last_chunk.data = data;
  }

 private:
   struct Chunk {
    uint64_t media_sequence_number = 0;
    int duration = 0;
    Bytes data;
  };

  std::vector<Chunk> chunks_;
  const float chunk_duration_;

  [[nodiscard]] http::Response HandleGet(const http::Request &request) const {
    http::Response response;

    if (request.url == kPlaylistPath) {
      response.code = 200;
      response.description = "OK";
      response.body = GetPlaylistContent();
      response.headers[kContentLengthHeaderName] =
          std::to_string(response.body.size());
    } else if (std::regex_match(request.url, kChunkPathRegex)) {
      return GetChunk(request);
    } else {
      return NotFoundResponse;
    }

    return response;
  }

  [[nodiscard]] http::Response GetChunk(const http::Request &request) const {
    const uint64_t chunk_number = ExtractChunkNumberFromUrl(request.url);

    auto it = std::find_if(chunks_.begin(), chunks_.end(),
                           [chunk_number] (const Chunk &chunk) {
                             return chunk.media_sequence_number == chunk_number;
                           });
    if (it == chunks_.end()) {
      return NotFoundResponse;
    }

    http::Response response;
    response.code = 200;
    response.description = "OK";
    response.body.insert(response.body.begin(), it->data.data(),
                         it->data.data() + it->data.size());
    response.headers[kContentLengthHeaderName] =
        std::to_string(response.body.size());

    return response;
  }

  [[nodiscard]] std::string GetPlaylistContent() const {
    using namespace std::string_literals;

    std::ostringstream oss(
        "#EXTM3U\n"
        "#EXT-X-VERSION:3\n"
        "#EXT-X-TARGETDURATION:"s + std::to_string(chunk_duration_) + "\n"
        "#EXT-X-MEDIA-SEQUENCE:"s +
            std::to_string(chunks_.front().media_sequence_number) + "\n");

    for (const auto &chunk : chunks_) {
      oss << "#EXTINF:" << chunk.duration << ",\n"
          << "/chunk" << chunk.media_sequence_number << ".ts\n";
    }

    return oss.str();
  }

  [[nodiscard]] static uint64_t ExtractChunkNumberFromUrl(
      const std::string &url) {
    uint64_t chunk_number = 0;
    std::smatch matches;

    if (std::regex_search(url, matches, kChunkPathRegex) &&
        matches.size() == 2) {
      chunk_number = std::stoull(matches[1].str());
    }

    return chunk_number;
  }
};

} // namespace hls