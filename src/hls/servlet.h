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
#include <mutex>

#include "servlet.h"
#include "http/request.h"
#include "http/response.h"
#include "observer.h"
#include "types/mpeg2ts_chunk.h"

namespace hls {

const char kContentLengthHeaderName[] = "Content-Length";
const char kPlaylistPath[] = "playlist.m3u";
const std::regex kChunkPathRegex("^chunk(\\d+)\\.ts$");
const http::Response NotFoundResponse = {404, "Not Found"};

class Servlet : public ::Servlet<http::Request, http::Response>,
                public Observer<types::Mpeg2TsChunk> {
 public:
  /**
   * @param chunk_count Number of chunk to store in memory
   * @param chunk_duration Max duration of one chunk in seconds
   */
  Servlet(int chunk_count, float chunk_duration):
  chunks_(chunk_count),
  cached_chunks_(chunk_count),
  chunk_duration_(chunk_duration),
  chunks_mutex_() {
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
  void Receive(const types::Mpeg2TsChunk &chunk) override {
    std::lock_guard guard(chunks_mutex_);
    std::cout << "HLS: Received " << chunk.media_sequence_number
              << " chunk" << std::endl;
    AppendNewChunk(cached_chunks_, chunks_.at(0));
    AppendNewChunk(chunks_, chunk);

    if (chunk.media_sequence_number == chunks_.size() - 1) {
      std::cout << "HLS: Ready" << std::endl;
    }
  }

 private:
  std::vector<types::Mpeg2TsChunk> chunks_;
  std::vector<types::Mpeg2TsChunk> cached_chunks_;
  const float chunk_duration_;
  mutable std::mutex chunks_mutex_;

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
    const std::vector<types::Mpeg2TsChunk> *chunks_ptr = &chunks_;
    if (chunk_number <= chunks_.back().media_sequence_number - chunks_.size()) {
      chunks_ptr = &cached_chunks_;
    }

    std::lock_guard guard(chunks_mutex_);
    auto it = FindChunk(*chunks_ptr, chunk_number);
    if (it == chunks_ptr->end()) {
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

    {
      std::lock_guard guard(chunks_mutex_);
      for (const auto &chunk : chunks_) {
        oss << "#EXTINF:" << chunk.duration << ",\n"
            << "/chunk" << chunk.media_sequence_number << ".ts\n";
      }
    }

    return oss.str();
  }

  static void AppendNewChunk(std::vector<types::Mpeg2TsChunk> chunks,
                             const types::Mpeg2TsChunk &chunk) {
    for (std::size_t i = 0; i < chunks.size() - 1; ++i) {
      chunks[i] = std::move(chunks[i + 1]);
    }
    chunks.back() = chunk;
  }

  static typename std::vector<types::Mpeg2TsChunk>::iterator
  FindChunk(std::vector<types::Mpeg2TsChunk> chunks,
            const uint64_t chunk_number) {
    return std::find_if(chunks.begin(), chunks.end(),
                        [chunk_number] (const types::Mpeg2TsChunk &chunk) {
                          return chunk.media_sequence_number == chunk_number;
                        });
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
