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

#include "client.h"

#include <netdb.h>
#include <arpa/inet.h>

#include <algorithm>
#include <iostream>

#include "request.h"
#include "sdp/session_description.h"
#include "split.h"
#include "rtp/packet.h"
#include "rtp/mjpeg/packet.h"

namespace {

/**
 * @brief Retrieve hostname and port from url
 * @throw std::invalid_argument, if provided bad url
 *
 * @param url Url to get hostname and ip from
 * @param default_port Default port, if it isn't specified in url
 * @return Pair of hostname and port
 */
std::pair<std::string, int> GetHostnameAndPort(const std::string &url,
                                               const int default_port) {
  std::string::size_type hostname_start = url.find("://");
  if ((hostname_start == std::string::npos) ||
      (hostname_start >= url.size() - 3)) {
    throw std::invalid_argument("Invalid url");
  }
  hostname_start += 3;

  std::string hostname = url.substr(hostname_start,
                                    url.find('/', hostname_start) - hostname_start);
  std::string::size_type port_start = hostname.find(':');
  int port = default_port;
  if (port_start != std::string::npos) {
    port = std::stoi(hostname.substr(port_start + 1));
    hostname = hostname.substr(0, port_start);
  }

  return {hostname, port};
}

/**
 * @brief Get ip from hostname
 * @throw std::invalid_argument, if provided bad hostname
 * @throw std::runtime_error, if can't get ip from hostname
 *
 * @param hostname Name of the host to get ip from
 * @return Ip of host
 */
std::string GetIp(const std::string &hostname) {
  hostent *host_ptr = gethostbyname(hostname.c_str());
  if (host_ptr == nullptr) {
    throw std::invalid_argument("Invalid hostname");
  }

  auto addr_list = reinterpret_cast<in_addr **>(host_ptr->h_addr_list);
  for (int i = 0; i < host_ptr->h_length; ++i) {
    in_addr *addr = addr_list[i];
    if (addr != nullptr) {
      return inet_ntoa(*addr_list[i]);
    }
  }

  throw std::runtime_error("Can't get host ip");
}

} // namespace

namespace rtsp {

using namespace std::string_literals;

Client::Client(std::string url):
url_(std::move(url)),
rtsp_socket_(sock::Type::kTcp),
rtp_socket_(sock::Type::kUdp, 4577),
session_description_(),
width_(0),
height_(0),
fps_(0),
session_id_(0),
rtp_data_receiving_worker_(),
worker_stop_(false),
worker_mutex_() {
  auto [hostname, port] = GetHostnameAndPort(url_, 554);
  std::string server_ip = GetIp(hostname);
  std::cout << "Connecting to " << server_ip << ":" << port << std::endl;
  if (!rtsp_socket_.Connect(server_ip, port)) {
    throw std::runtime_error("Can't connect to the RTSP server "s +
                             server_ip + ':' + std::to_string(port));
  }

  HandleOptionsResponse(SendOptionsRequest());
  HandleDescribeResponse(SendDescribeRequest());
  HandleSetupResponse(SendSetupRequest());

  (void)SendPlayRequest();
  rtp_data_receiving_worker_ = std::thread(&Client::RtpDataReceiving, this);
}

Client::~Client() {
  (void)SendTeardownRequest();

  {
    std::lock_guard lock(worker_mutex_);
    worker_stop_ = true;
  }
  rtp_data_receiving_worker_.join();
}

int Client::GetWidth() const {
  return width_;
}

int Client::GetHeight() const {
  return height_;
}

int Client::GetFps() const {
  return fps_;
}

Response Client::SendOptionsRequest() {
  Request request = BuildRequestSkeleton(Method::kOptions);
  SendRequest(request);

  return ReceiveResponse();
}

void Client::HandleOptionsResponse(const Response &response) {
  const char kPublicHeader[] = "Public";
  if (!response.headers.count(kPublicHeader)) {
    throw std::runtime_error("Server did not send acceptable methods");
  }
  VerifyAcceptableMethods(Split(response.headers.at(kPublicHeader), ", "));
}

Response Client::SendDescribeRequest() {
  Request request = BuildRequestSkeleton(Method::kDescribe);
  request.headers["Accept"] = "application/sdp";
  SendRequest(request);

  return ReceiveResponse();
}

void Client::HandleDescribeResponse(const Response &response) {
  session_description_ = sdp::ParseSessionDescription(response.body);
  auto video_description_it =
      FindVideoMediaDescription(session_description_.media_descriptions);
  if (video_description_it == session_description_.media_descriptions.end()) {
    throw std::runtime_error(
        "There is no required \"video\" media description in server's SDP");
  }
  auto last_char_it = std::prev(url_.end());
  if (*(last_char_it) == '/') {
    url_.erase(last_char_it);
  }
  url_ += ExtractVideoPath(*video_description_it);
  std::tie(width_, height_) = ExtractDimensions(*video_description_it);
  fps_ = ExtractFps(*video_description_it);
}

Response Client::SendSetupRequest() {
  Request request = BuildRequestSkeleton(Method::kSetup);
  const int port_number = rtp_socket_.GetPortNumber();
  request.headers["Transport"] =
      "RTP/AVP;unicast;client_port="s + std::to_string(port_number) + "-"s +
      std::to_string(port_number + 1);
  SendRequest(request);

  return ReceiveResponse();
}

void Client::HandleSetupResponse(const Response &response) {
  const char kTransportHeader[] = "Transport";
  if (!response.headers.count(kTransportHeader) ||
      response.headers.at(kTransportHeader).find("RTP/AVP") == std::string::npos) {
    throw std::runtime_error("Server doesn't allow RTP/AVP translation");
  }

  const char kSessionHeader[] = "Session";
  if (!response.headers.count(kSessionHeader)) {
    throw std::runtime_error(
        "Server's response on SETUP request doesn't have Session header");
  }
  session_id_ = std::stoul(response.headers.at(kSessionHeader));
}

Response Client::SendPlayRequest() {
  Request request = BuildRequestSkeleton(Method::kPlay);
  request.headers["Range"] = "npt=0.000-";
  request.headers["Session"] = std::to_string(session_id_);
  SendRequest(request);

  return ReceiveResponse();
}

Response Client::SendTeardownRequest() {
  Request request = BuildRequestSkeleton(Method::kTeardown);
  request.headers["Session"] = std::to_string(session_id_);
  SendRequest(request);

  return ReceiveResponse();
}

Request Client::BuildRequestSkeleton(const Method method) {
  static uint32_t cseq_counter = 0;

  Request request;
  request.method = method;
  request.url = url_;
  request.version = 1.0;
  request.headers["Cseq"] = std::to_string(++cseq_counter);
  request.headers["User-Agent"] = "Arjentix Media Server";

  return request;
}

void Client::SendRequest(const Request &request) {
  std::cout << "\nRequest:\n" << request << std::endl;
  rtsp_socket_ << request << std::endl;
}

Response Client::ReceiveResponse() {
  Response response;
  rtsp_socket_ >> response;
  std::cout << "\nResponse:\n" << response << std::endl;
  VerifyResponseIsOk(response);

  return response;
}

void Client::RtpDataReceiving() {
  std::vector<rtp::mjpeg::Packet> mjpeg_packets;

  for (;;) {
    {
      std::lock_guard lock(worker_mutex_);
      if (worker_stop_) {
        return;
      }
    }

    rtp::Packet rtp_packet;
    rtp_socket_ >> rtp_packet;
    rtp::mjpeg::Packet mjpeg_packet;
    mjpeg_packet.Deserialize(rtp_packet.payload);
    mjpeg_packets.push_back(std::move(mjpeg_packet));
    if (rtp_packet.header.marker == 1U) {
      types::Bytes frame = rtp::mjpeg::UnpackJpeg(mjpeg_packets);
      try {
        ProvideToAll(types::MjpegFrame(frame));
      } catch (std::runtime_error &ex) {
        std::cout << "Warning: " << ex.what() << std::endl;
      }
      mjpeg_packets.clear();
    }
  }
}

std::vector<sdp::MediaDescription>::const_iterator Client::FindVideoMediaDescription(
    const std::vector<sdp::MediaDescription> &media_descriptions) {
  return std::find_if(media_descriptions.begin(), media_descriptions.end(),
                      [] (const sdp::MediaDescription &media_description) {
                        return media_description.name.find("video") != std::string::npos;
                      });
}

std::string Client::ExtractVideoPath(const sdp::MediaDescription &description) {
  const auto &attributes = description.attributes;
  auto control_it = std::find_if(attributes.begin(), attributes.end(),
                                 [] (const sdp::Attribute &attr) {
                                   return attr.first == "control";
                                 });
  if (control_it == attributes.end()) {
    return "";
  }

  return ("/"s + control_it->second);
}

std::pair<int, int> Client::ExtractDimensions(
    const sdp::MediaDescription &description) {
  const auto &attributes = description.attributes;
  auto cliprect_it = std::find_if(attributes.begin(), attributes.end(),
                                  [] (const sdp::Attribute &attr) {
                                    return attr.first == "cliprect";
                                  });
  const std::string full_description_name = description.name +
                                            R"(" media description)";
  if (cliprect_it == attributes.end()) {
    throw std::runtime_error(
        R"(There is no required "cliprect" attribute in ")" +
        full_description_name);
  }

  const std::string &cliprect = cliprect_it->second;
  const std::string::size_type last_coma_pos = cliprect.rfind(',');
  const std::string::size_type pre_last_coma_pos =
      cliprect.rfind(',', last_coma_pos - 1);

  if ((last_coma_pos == std::string::npos) ||
      (pre_last_coma_pos == std::string::npos)) {
    throw std::runtime_error(R"(Invalid "cliprect" attribute in ")" +
                             full_description_name);
  }
  int width = std::stoi(cliprect.substr(last_coma_pos + 1));
  int height = std::stoi(cliprect.substr(pre_last_coma_pos + 1,
                                         last_coma_pos - pre_last_coma_pos - 1));

  return {width, height};
}

int Client::ExtractFps(const sdp::MediaDescription &description) {
  const auto &attributes = description.attributes;
  auto framerate_it = std::find_if(attributes.begin(), attributes.end(),
                                  [] (const sdp::Attribute &attr) {
                                    return attr.first == "framerate";
                                  });
  const std::string full_description_name = description.name +
                                            R"(" media description)";
  if (framerate_it == attributes.end()) {
    throw std::runtime_error(
        R"(There is no required "cliprect" attribute in ")" +
        full_description_name);
  }

  return std::stoi(framerate_it->second);
}

void Client::VerifyResponseIsOk(const Response &response) {
  if (response.code != 200) {
    throw std::runtime_error("Response has error code "s +
        std::to_string(response.code) + ": " + response.description);
  }
}

void Client::VerifyAcceptableMethods(
    const std::vector<std::string> &acceptable_methods_strings) {
  const std::vector<Method> kRequiredMethods =
      {Method::kDescribe, Method::kSetup, Method::kPlay, Method::kTeardown};

  for (auto method : kRequiredMethods) {
    const std::string method_str = http::MethodToString(method);
    auto it = std::find(acceptable_methods_strings.begin(),
                        acceptable_methods_strings.end(),
                        method_str);
    if (it == acceptable_methods_strings.end()) {
      throw std::runtime_error("Server doesn't accept required "s + method_str + " method");
    }
  }
}

} // namespace rtsp
