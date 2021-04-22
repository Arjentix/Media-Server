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

#include "frame_provider.h"

#include <memory>
#include <unordered_map>

/**
 * @brief Class for registering and getting frame provider
 */
class FrameProviderManager {
 public:
  /**
   * @brief Register frame provider
   *
   * @param source_id Id of video source
   * @param codec_id Id of frames coded
   * @param frame_provider_ptr Pointer to the frame provider to register
   */
  void Register(const std::string &source_id, const std::string &codec_id,
                std::shared_ptr<FrameProvider> frame_provider_ptr);

  /**
   * @brief Get frame provider by given source and codes Ids
   *
   * @throw std::out_of_range if can't find suitable provider
   *
   * @param source_id Id of video source
   * @param codec_id Id of frames coded
   * @return frame_provider_ptr Pointer to the frame provider
   */
  std::shared_ptr<FrameProvider> GetProvider(const std::string &source_id,
                                             const std::string &codec_id) const;

 private:
  std::unordered_map<std::string, std::shared_ptr<FrameProvider>>
      sourceToProvider_;
};
