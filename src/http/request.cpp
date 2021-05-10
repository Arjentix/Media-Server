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

#include "request.h"

namespace http {

template <>
Method ParseMethod<Method>(const std::string &method_str) {
  Method method;

  if (method_str == "OPTIONS") {
    method = Method::kOptions;
  } else if (method_str == "GET") {
    method = Method::kGet;
  } else if (method_str == "HEAD") {
    method = Method::kHead;
  } else if (method_str == "POST") {
    method = Method::kPost;
  } else if (method_str == "PUT") {
    method = Method::kPut;
  } else if (method_str == "DELETE") {
    method = Method::kDelete;
  } else {
    throw ParseError("Unknown method " + method_str);
  }

  return method;
}

template <>
std::string MethodToString<Method>(Method method) {
  std::string method_str;

  switch (method) {
    case Method::kOptions:
      method_str = "OPTIONS";
      break;
    case Method::kGet:
      method_str = "GET";
      break;
    case Method::kHead:
      method_str = "HEAD";
      break;
    case Method::kPost:
      method_str = "POST";
      break;
    case Method::kPut:
      method_str = "PUT";
      break;
    case Method::kDelete:
      method_str = "DELETE";
      break;
    default:
      method_str = "UNKNOWN METHOD";
  }

  return method_str;
}

} // namespace http
