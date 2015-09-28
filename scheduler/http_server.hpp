/**
 * Copyright 2015 Quobyte Inc. All rights reserved.
 *
 * See LICENSE file for license details.
 */

#pragma once

#include <cstdint>
#include <string>
#include <functional>

struct MHD_Daemon;
struct MHD_Connection;

namespace quobyte {

// status server displaying error log and metrics
class HttpServer {
 public:
  typedef
      std::function<
          std::string(const std::string&, const std::string&, const std::string&)>
      Dispatcher;

  HttpServer(int port);
  void Start(Dispatcher request_dispatcher);
  void Stop();

  int port() const;

 private:
  static int HandleRequest(void *cls,
                           struct MHD_Connection* connection,
                           const char* url,
                           const char* method,
                           const char* version,
                           const char* upload_data,
                           size_t* update_data_size,
                           void** ptr);
  struct MHD_Daemon* daemon_ = NULL;
  Dispatcher request_dispatcher_;
  int port_;
};

}  // namespace quobyte
