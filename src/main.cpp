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

#include <csignal>

#include <iostream>
#include <memory>

#include "port_handler/port_handler.h"
#include "port_handler/port_handler_manager.h"
#include "rtsp/client.h"
#include "converters/mjpeg_to_h264.h"
#include "converters/mpeg2ts_packager.h"
#include "hls/servlet.h"

namespace {

volatile bool stop_flag = false;

void SignalHandler(int) {
  stop_flag = true;
}

class MediaServer {
 public:
  explicit MediaServer(const std::string &rtsp_stream_url):
  rtsp_client_(rtsp_stream_url),
  mjpeg_to_h264_ptr_(),
  mpeg2ts_packager_ptr_(),
  port_handler_manager_() {
    const int width = rtsp_client_.GetWidth();
    const int height = rtsp_client_.GetHeight();
    const int fps = rtsp_client_.GetFps();

    mjpeg_to_h264_ptr_ = std::make_shared<converters::MjpegToH264>
        (width, height, fps);
    mpeg2ts_packager_ptr_ = std::make_shared<converters::Mpeg2TsPackager>
        (width, height, fps, kHlsChunkDurationSec);
    rtsp_client_.AddObserver(mjpeg_to_h264_ptr_);
    mjpeg_to_h264_ptr_->AddObserver(mpeg2ts_packager_ptr_);

    port_handler_manager_.RegisterPortHandler(BuildHlsPortHandler());
  }

  void Start() {
    std::cout << "Media server started" << std::endl;

    const int kAcceptTimeoutInMilliseconds = 2000;
    while (!stop_flag) {
      port_handler_manager_.TryAcceptClients(kAcceptTimeoutInMilliseconds);
    }
  }

 private:
  static constexpr int kHlsPort = 8080;
  static constexpr int kHlsChunkCount = 3;
  static constexpr float kHlsChunkDurationSec = 8.0;

  rtsp::Client rtsp_client_;
  std::shared_ptr<converters::MjpegToH264> mjpeg_to_h264_ptr_;
  std::shared_ptr<converters::Mpeg2TsPackager> mpeg2ts_packager_ptr_;
  port_handler::PortHandlerManager port_handler_manager_;

  /**
   * @brief Create HLS handler
   *
   * @return Pointer to PortHandlerBase with HLS port handler inside
   */
  std::unique_ptr<port_handler::PortHandlerBase> BuildHlsPortHandler() {
    auto hls_port_handler_ptr = std::make_unique<
        port_handler::PortHandler<http::Request, http::Response>>(kHlsPort);

    auto servlet_ptr = std::make_shared<hls::Servlet>(kHlsChunkCount, kHlsChunkDurationSec);
    mpeg2ts_packager_ptr_->AddObserver(servlet_ptr);

    hls_port_handler_ptr->RegisterServlet("/", servlet_ptr);

    return hls_port_handler_ptr;
  }
};

} // namespace

int main(int argc, char **argv) {
  try {
    signal(SIGINT, SignalHandler);
    signal(SIGTERM, SignalHandler);

    if (argc < 2) {
      std::cerr << "Usage: " << argv[0] << " <rtsp-stream-url>" << std::endl;
      return EXIT_FAILURE;
    }

    MediaServer media_server(argv[1]);
    media_server.Start();
  } catch (const std::exception &ex) {
    std::cerr << "Error: " << ex.what() << std::endl;
    return EXIT_FAILURE;
  } catch (...) {
    std::cerr << "Unknown error occurred" << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
