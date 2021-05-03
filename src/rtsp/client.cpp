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

#include <algorithm>
#include <iostream>

#include "request.h"
#include "sdp/session_description.h"
#include "split.h"
#include "rtp/packet.h"
#include "rtp/mjpeg/packet.h"

namespace rtsp {

using namespace std::string_literals;

Client::Client(const std::string &server_ip, const int port, std::string url):
url_(std::move(url)),
rtsp_socket_(sock::Type::kTcp),
rtp_socket_(sock::Type::kUdp, 4577),
session_description_(),
session_id_(0),
rtp_data_receiving_worker_(),
worker_stop_(false),
worker_mutex_() {
  if (!rtsp_socket_.Connect(server_ip, port)) {
    throw std::runtime_error("Can't connect to the RTSP server "s +
                             server_ip + ':' + std::to_string(port));
  }

  const char kPublicHeader[] = "Public";
  Response response = SendOptionsRequest();
  if (!response.headers.count(kPublicHeader)) {
    throw std::runtime_error("Server did not send acceptable methods");
  }
  VerifyAcceptableMethods(Split(response.headers.at(kPublicHeader), ", "));

  response = SendDescribeRequest();
  session_description_ = sdp::ParseSessionDescription(response.body);
  AppendVideoPathInUrl();

  response = SendSetupRequest();
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

Response Client::SendOptionsRequest() {
  Request request = BuildRequestSkeleton(Method::kOptions);
  SendRequest(request);

  return ReceiveResponse();
}

Response Client::SendDescribeRequest() {
  Request request = BuildRequestSkeleton(Method::kDescribe);
  request.headers["Accept"] = "application/sdp";
  SendRequest(request);

  return ReceiveResponse();
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

void Client::AppendVideoPathInUrl() {
  const auto &media_descriptions = session_description_.media_descriptions;
  auto video_it = std::find_if(media_descriptions.begin(), media_descriptions.end(),
      [] (const sdp::MediaDescription &media_description) {
        return media_description.name.find("video") != std::string::npos;
      });
  if (video_it == media_descriptions.end()) {
    return;
  }

  const auto &attributes = video_it->attributes;
  auto control_it = std::find_if(attributes.begin(), attributes.end(),
      [] (const sdp::Attribute &attr) {
        return attr.first == "control";
      });
  if (control_it == attributes.end()) {
    return;
  }

  url_ += "/"s + control_it->second;
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
//      Bytes frame = rtp::mjpeg::PackToJpeg(mjpeg_packets);
//      ProvideToAll(frame);
      mjpeg_packets.clear();
    }
  }
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
    const std::string method_str = MethodToString(method);
    auto it = std::find(acceptable_methods_strings.begin(),
                        acceptable_methods_strings.end(),
                        method_str);
    if (it == acceptable_methods_strings.end()) {
      throw std::runtime_error("Server doesn't accept required "s + method_str + " method");
    }
  }
}

} // namespace rtsp
