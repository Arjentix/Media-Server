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
#include <string_view>
#include <map>
#include <unordered_set>
#include <memory>
#include <regex>

#include "servlet.h"

namespace port_handler {
/**
 * @brief RequestDispatcher allows to register handlers and then dispatch RTSP
 * request to the qualified handler returning RTSP response
 */
template <typename RequestType, typename ResponseType>
class RequestDispatcher {
 public:
  using ServletPtr = std::shared_ptr<Servlet<RequestType, ResponseType>>;

  RequestDispatcher():
  url_to_servlet_(),
  acceptable_urls_() {
  }

  /**
   * @brief Register new servlet, that will process request on specified url
   *
   * @param url Unique url
   * @param servlet_ptr Pointer to the Servlet inheritor
   * @return Reference to this
   */
  RequestDispatcher &RegisterServlet(std::string url,
                                     ServletPtr servlet_ptr) {
    auto res = url_to_servlet_.insert({std::move(url), servlet_ptr});
    if (res.second) {
      acceptable_urls_.insert(res.first->first);
    }

    return *this;
  }

  /**
   * @brief Dispatch request to the specified servlet and get a response
   * @details Must specific servlet will be chosen according to the url
   *
   * @param request Request to dispatch
   * @return Response from servlet if such was found
   * @return Response with error in other way
   */
  ResponseType Dispatch(RequestType request) const {
    try {
      auto [path, servlet_ptr] = *ChooseServlet(request.url);
      request.url = ExtractPath(request.url).substr(path.size());
      return servlet_ptr.Handle(request);
    }
    catch (const BadUrl &ex) {
      return {400, "Bad Request"};
    }
    catch (const std::out_of_range &ex) {
      return {404, "Not Found"};
    }
    catch (const std::runtime_error &ex) {
      return {500, "Internal Server Error"};
    }
  }

 private:
  using UrlToServletMap = std::map<std::string, ServletPtr>;

  //! Url -> Servlet inheritor
  UrlToServletMap url_to_servlet_;
  //! All acceptable urls
  std::unordered_set<std::string_view> acceptable_urls_;

  /**
   * @brief Choose proper Servlet to serve request on the given url
   *
   * @param url Request url
   * @return Iterator, pointing on {Servlet url, Servlet} pair
   */
  typename UrlToServletMap::const_iterator ChooseServlet(const std::string &url) const {
    if (url_to_servlet_.size() == 0) {
      throw std::out_of_range("There aren't any servlets at all");
    }

    const std::string path = ExtractPath(url);
    auto it = url_to_servlet_.lower_bound(path);
    if ((it != url_to_servlet_.end()) &&
        (it->first == path)) {
      return it;
    }

    if (it != url_to_servlet_.begin()) {
      --it;
      if (path.find(it->first) == 0) {
        return it;
      }
    }

    throw std::out_of_range("Can't find suitable servlet");
  }

  /**
   * @brief Exception, that indicates that invalid url was received
   */
  class BadUrl : public std::runtime_error {
   public:
    explicit BadUrl(std::string_view message) :
        std::runtime_error(message.data()) {}
  };

  /**
   * @brief Extract path from url
   *
   * @param full_url A full url with protocol, hostname and etc.
   * @return Path from the url
   */
  static std::string ExtractPath(const std::string &full_url) {
    const uint kGroupsNumber = 7;
    const std::string scheme = R"(^(\S+)://)";
    const std::string login = R"(([^:\s]+)?)";
    const std::string password = R"((\S+))";
    const std::string
        login_password = R"((?:)" + login + R"((?::)" + password + R"()?@)?)";
    const std::string hostname = R"(([^:/]+))";
    const std::string port = R"((?::([0-9]+))?)";
    const std::string path = R"(((?:/[^/\s]+)+))";
    const std::regex reg_expr(scheme + login_password + hostname + port + path);
    std::smatch matches;

    if (!std::regex_search(full_url, matches, reg_expr) ||
        (matches.size() < kGroupsNumber)) {
      throw BadUrl("Bad url");
    }

    std::string path_value = matches[kGroupsNumber - 1].str();
    if (*(path_value.rend()) == '/') {
      path_value = path_value.substr(0, path_value.size() - 1);
    }

    return path_value;
  }
};

} // namespace port_handler
