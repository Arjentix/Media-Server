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

#include "frame_provider.h"
#include "frame_provider_manager.h"
#include "port_handler/port_handler.h"

namespace {

volatile bool stop_flag = false;

void SignalHandler(int) {
  stop_flag = true;
}

/**
 * @brief Create frame provider manager with some registered providers
 *
 * @return Frame provider manager with registered providers
 */
FrameProviderManager BuildFrameProviderManager() {
  FrameProviderManager frame_provider_manager;

  const std::string kMjpegRtspSourceIp = "192.168.0.16";
  const int kMjpegRtspSourcePort = 5544;
  auto mjpeg_receiver_ptr = std::make_shared<MjpegOverRtspReceiver>(
      kMjpegRtspSourceIp, kMjpegRtspSourcePort);

  auto h264_converter_ptr = std::make_shared<MjpegToH264Converter>(mjpeg_receiver_ptr);

  frame_provider_manager.Register("source1", "MJPEG", mjpeg_receiver_ptr)
                        .Register("source1", "H.264", h264_converter_ptr);
  return frame_provider_manager;
}

/**
 * @brief Create HLS handler on port 2020, that takes frames of source identified by source_id
 *
 * @param source_id Id of video source for GlobalFrameProvidersManager
 * @return Pointer to PortHandlerBase with HLS port handler inside
 */
std::unique_ptr<port_handler::PortHandlerBase> BuildHlsPortHandler(
    const FrameProviderManager &frame_provider_manager,
    const std::string &source_id) {
  const int kHlsPort = 2020;
  auto hls_port_handler_ptr = std::make_unique<
      port_handler::PortHandler<hls::Request, hls::Response>>(kHlsPort);

  std::shared_ptr<FrameProvider> h264_provider_ptr =
       frame_provider_manager.GetProvider(source_id, "H.264");

  hls_port_handler_ptr->RegisterServlet(
      "/h264",
      hls::H264Servlet(h264_provider_ptr));

  return hls_port_handler_ptr;
}

/**
 * @brief Create manager and register all port handlers
 *
 * @return PortHandlerManager with all handlers registered
 */
port_handler::PortHandlerManager BuildPortHandlerManager(
    const FrameProviderManager &frame_provider_manager) {
  port_handler::PortHandlerManager port_handler_manager;

  port_handler_manager.RegisterPortHandler(
      BuildHlsPortHandler(frame_provider_manager, "source1"));

  return port_handler_manager;
}

} // namespace

int main() {
  try {
    signal(SIGINT, SignalHandler);

    FrameProviderManager frame_provider_manager = BuildFrameProviderManager();
    port_handler::PortHandlerManager port_handler_manager =
        BuildPortHandlerManager(frame_provider_manager);

    const int kAcceptTimeoutInMilliseconds = 2000;
    while (!stop_flag) {
      port_handler_manager.TryAcceptClients(kAcceptTimeoutInMilliseconds);
    }
  } catch (const std::exception &ex) {
    std::cerr << "Error: " << ex.what() << std::endl;
    return EXIT_FAILURE;
  } catch (...) {
    std::cerr << "Unknown error occurred" << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
