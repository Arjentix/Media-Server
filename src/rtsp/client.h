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

#include "frame/provider.h"
#include "sock/client_socket.h"
#include "sock/server_socket.h"
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

 private:
  std::random_device rd_; //!< Device for random rtp port generating
  //! Engine for random rtp port generating
  std::default_random_engine random_engine_;
  //! Distribution for random rtp port generating
  std::uniform_int_distribution<int> distribution_;
  std::string url_; //!< RTSP content url
  sock::ClientSocket rtsp_socket_; //!< Socket for RTSP TCP connection
  sock::ServerSocket rtp_socket_; //!< Socket for RTP UDP data receiving
  //! Session description
  sdp::SessionDescription session_description_;
  uint32_t session_id_; //!< Session identifier

  /**
   * @brief Send OPTION request to the server
   *
   * @return Response from server
   */
  Response SendOptionsRequest();

  /**
   * @brief Send DESCRIBE request to the server
   *
   * @return Response from server
   */
  Response SendDescribeRequest();

  /**
   * @brief Send SETUP request to the server
   *
   * @return Response from server
   */
  Response SendSetupRequest();

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
   * @brief Append path to the video track in the url using session_description_
   */
  void AppendVideoPathInUrl();

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

  /**
   * @brief Log request to the cout
   *
   * @param request Request to log
   */
  static void LogRequest(const Request &request);

  /**
   * @brief Log response to the cout
   *
   * @param response Response to log
   */
  static void LogResponse(const Response &response);
};

} // namespace rtsp
