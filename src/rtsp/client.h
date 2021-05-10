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

#include <string>
#include <random>
#include <thread>
#include <mutex>

#include "frame/provider.h"
#include "sock/client_socket.h"
#include "sock/server_socket.h"
#include "request.h"
#include "response.h"
#include "sdp/session_description.h"

namespace rtsp {

/**
 * @brief RTSP client, that connects to the RTSP server and provides frames
 */
class Client : public frame::Provider {
 public:
  /**
   * @details Blocks until connection is established
   *
   * @param server_ip RTSP server's ip address
   * @param port RTSP server's port
   * @param url Url to send request on
   */
  Client(const std::string &server_ip, int port, std::string url);

  /**
   * @brief Sends TEARDOWN request
   */
  ~Client();

  /**
   * @brief Get image width
   *
   * @return Width
   */
  int GetWidth() const;

  /**
   * @brief Get image height
   *
   * @return Height
   */
  int GetHeight() const;

  /**
   * @brief Get video fps
   *
   * @return Fps
   */
  int GetFps() const;

 private:
  std::string url_; //!< RTSP content url
  sock::ClientSocket rtsp_socket_; //!< Socket for RTSP TCP connection
  sock::ServerSocket rtp_socket_; //!< Socket for RTP UDP data receiving
  //! Session description
  sdp::SessionDescription session_description_;
  int width_; //!< Image width
  int height_; //!< Image height
  int fps_; //!< Video fps
  uint32_t session_id_; //!< Session identifier
  //! Worker that receives data on rtp_socket_ and provide it to all observers
  std::thread rtp_data_receiving_worker_;
  bool worker_stop_; //!< True, if rtp_data_receiving_worker_ should stop
  std::mutex worker_mutex_; //!< Mutex for rtp_data_receiving_worker_

  /**
   * @brief Send OPTION request to the server
   *
   * @return Response from server
   */
  Response SendOptionsRequest();

  /**
   * @brief Handle server's response to the OPTIONS request
   *
   * @param response Server's response to the OPTIONS request
   */
  void HandleOptionsResponse(const Response &response);

  /**
   * @brief Send DESCRIBE request to the server
   *
   * @return Response from server
   */
  Response SendDescribeRequest();

  /**
   * @brief Handle server's response to the DESCRIBE request
   *
   * @param response Server's response to the DESCRIBE request
   */
  void HandleDescribeResponse(const Response &response);

  /**
   * @brief Send SETUP request to the server
   *
   * @return Response from server
   */
  Response SendSetupRequest();

  /**
   * @brief Handle server's response to the SETUP request
   *
   * @param response Server's response to the SETUP request
   */
  void HandleSetupResponse(const Response &response);

  /**
   * @brief Send PLAY request to the server
   *
   * @return Response from server
   */
  Response SendPlayRequest();

  /**
   * @brief Send TEARDOWN request to the server
   *
   * @return Response from server
   */
  Response SendTeardownRequest();

  /**
   * @brief Build basic request skeleton. CSeq counting is done automatically
   *
   * @param method RTSP method
   * @return Request skeleton
   */
  Request BuildRequestSkeleton(Method method);

  /**
   * @brief Send RTSP request and log it
   *
   * @param request Request to send
   */
  void SendRequest(const Request &request);

  /**
   * @brief Receive RTSP response from rtsp_socket_, verify it and log it
   *
   * @return Received response
   */
  Response ReceiveResponse();

  /**
   * @brief Receive data on rtp_socket_, pack it to jpeg and provide to all observers
   */
  void RtpDataReceiving();

  /**
   * @brief Find "video" media description
   *
   * @return Iterator, pointing to the "video" media description
   * @return media_descriptions.end(), if no such media description
   */
  static std::vector<sdp::MediaDescription>::const_iterator FindVideoMediaDescription(
      const std::vector<sdp::MediaDescription> &media_descriptions);

  /**
   * @brief Extract video path from SDP Media Description
   *
   * @param description SDP Media Description
   * @return Path to the video media with leading '/'
   * @return "" if can' find video media description
   */
  static std::string ExtractVideoPath(const sdp::MediaDescription &description);

  /**
   * @brief Extract image width and height from SDP Media Description
   * @throw std::runtime_error if can't find "cliprect" attribute in description
   *
   * @param description SDP Media Description
   * @return Pair of width and height
   */
  static std::pair<int, int> ExtractDimensions(const sdp::MediaDescription &description);

  /**
   * @brief Extract video fps from SDP Media Description
   * @throw std::runtime_error if can't find "framerate" attribute in description
   *
   * @param description SDP Media Description
   * @return Video fps
   */
  static int ExtractFps(const sdp::MediaDescription &description);

  /**
   * @brief Check if response has 200 status code
   *
   * @throws std::runtime_error If response has status different than 200
   *
   * @param response RTSP response to check
   */
  static void VerifyResponseIsOk(const Response &response);

  /**
   * @brief Check if all required methods are presented in acceptable methods
   * from OPTIONS request
   *
   * @throws std::runtime_error If one of required methods is not presented
   *
   * @param acceptable_methods_strings Vector of acceptable methods in string form
   */
  static void VerifyAcceptableMethods(
      const std::vector<std::string> &acceptable_methods_strings);
};

} // namespace rtsp
