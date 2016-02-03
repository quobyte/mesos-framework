/**
 * Copyright 2015 Quobyte Inc. All rights reserved.
 *
 * See LICENSE file for license details.
 */

#include "http_server.hpp"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <functional>

#include <sys/socket.h>  // Defines 'socklen_t' for microhttpd.h (Ubuntu 12.04).
#include <microhttpd.h>

#include <glog/logging.h>

namespace quobyte {

HttpServer::HttpServer(int port) : port_(port) {}

void HttpServer::Start(Dispatcher request_dispatcher) {
  request_dispatcher_ = request_dispatcher;
  daemon_ = MHD_start_daemon(MHD_USE_THREAD_PER_CONNECTION,
                               port_,
                               NULL,
                               NULL,
                               &HttpServer::HandleRequest,
                               &request_dispatcher_,
                               MHD_OPTION_CONNECTION_TIMEOUT, 1,
                               MHD_OPTION_END);
  if (daemon_ == NULL) {
    LOG(FATAL) << "Could not start HTTP server.";
  }
}

void HttpServer::Stop() {
  if (daemon_ != NULL) {
    MHD_stop_daemon(daemon_);
    daemon_ = NULL;
  }
}

int HttpServer::port() const {
  return port_;
}

int HttpServer::HandleRequest(void *cls,
                              struct MHD_Connection* connection,
                              const char* url,
                              const char* method,
                              const char* version,
                              const char* upload_data,
                              size_t* upload_data_size,
                              void** ptr) {
  Dispatcher& dispatch = *static_cast<Dispatcher*>(cls);
  static int dummy;
  struct MHD_Response * response;
  int ret;

  if (&dummy != *ptr) {
    /* The first time only the headers are valid,
       do not respond in the first round... */
    *ptr = &dummy;
    return MHD_YES;
  }
  *ptr = NULL; /* clear context pointer */

  std::string post_body;
  if (*upload_data_size > 0) {
    post_body = std::string(upload_data, *upload_data_size);
  }
  std::string page = dispatch(method, url, post_body);
  response = MHD_create_response_from_data(page.length(),
                                           (void*) page.c_str(),
                                           MHD_NO,
                                           MHD_YES);
  ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
  MHD_destroy_response(response);
  return ret;
}
}  // namespace quobyte

