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

namespace {

const char kCseq[] = "Cseq";

} // namespace

namespace rtsp {

using namespace std::string_literals;

Client::Client(const std::string &server_ip, const int port, std::string url):
rd_(),
random_engine_(rd_()),
distribution_(1024, 10000),
url_(std::move(url)),
rtsp_socket_(sock::Type::kTcp),
rtp_socket_(sock::Type::kUdp, distribution_(random_engine_))
{
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
  sdp::SessionDescription session_description = sdp::ParseSessionDescription(response.body);
}

Response Client::SendOptionsRequest() {
  Request request = BuildRequestSkeleton(Method::kOptions);
  SendRequest(request);

  return ReceiveResponse();
}

Response Client::SendDescribeRequest() {
  Request request = BuildRequestSkeleton(Method::kDescribe);
  request.headers.insert({"Accept", "application/sdp"});
  SendRequest(request);

  return ReceiveResponse();
}

Request Client::BuildRequestSkeleton(const Method method) {
  static uint32_t cseq_counter = 0;

  Request request;
  request.method = method;
  request.url = url_;
  request.version = 1.0;
  request.headers.insert({kCseq, std::to_string(++cseq_counter)});
  request.headers.insert({"User-Agent", "Arjentix Media Server"});

  return request;
}

void Client::SendRequest(const Request &request) {
  LogRequest(request);
  rtsp_socket_ << request << std::endl;
}

Response Client::ReceiveResponse() {
  Response response;
  rtsp_socket_ >> response;
  VerifyResponseIsOk(response);
  LogResponse(response);

  return response;
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

void Client::LogRequest(const Request &request) {
  std::cout << "\nRequest:\n" << request << std::endl;
}

void Client::LogResponse(const Response &response) {
  std::cout << "\nResponse:\n" << response << std::endl;
}

} // namespace rtsp
