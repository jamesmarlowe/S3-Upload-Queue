//
/// \file responder.h
/// \author James Marlowe
/// \brief Functions related to responding to a request made to Curl-Queue
//

#include <iostream>
#include <set>
#include <string>
#include <thread>
#include <sstream>
#include "zmq/zmqcpp.h"
#include <curl/curl.h>
#include <fstream>
#include <ctime>
#include <openssl/hmac.h>
#include "s3_upload.pb.h"

const char ROUTER[] = "tcp://localhost:5556";

//
/// \brief libcurl callback for CURLOPT_WRITEFUNCTION
/// \param buf: a pointer to the data that curl has for us
/// \param size*nmemb is the size of the buffer
/// \return length of written data
//
static size_t data_write(char* buf, size_t size, size_t nmemb, void* userp);

//
/// \brief libcurl callback for CURLOPT_WRITEFUNCTION
/// \param url: the url of the resource to request
/// \param timeout: how long to wait for the resource in seconds
/// \return curl status code
//
CURLcode curl_get(const std::string& url, struct curl_slist *headerlist, const char *post_body, long timeout);

//
/// \brief libcurl callback for CURLOPT_WRITEFUNCTION
/// \param url: the url of the resource to request
/// \param timeout: how long to wait for the resource in seconds
/// \return curl status code
//
CURLcode curl_upload(const std::string& url, struct curl_slist *headerlist, const char *post_body, long timeout);

//
/// \brief curls a url and gets response
/// \param req: protobuf containing request/response
/// \return bool success
//
void upload(S3Upload& req);

//
/// \brief Takes a request, and curls external resource for it
/// \pre TODO: figure out pres
/// \post Sends annotated data over ZeroMQ socket to original requester
void responder ();

//
/// \brief Annotates the request data
/// \pre TODO: figure out pres 
/// \post populates the request with data from curl
//
void upload(S3Upload& req);

/*!
 * \brief launches the program as a daemonize
 * \pre None
 * \post forks a process, and kills the current process
 */
void daemonize();

/*!
 * \brief Custom writer function for glog stack traces
 * \pre None
 * \post Message written to log file
 */
void write_stack_trace_to_log(const char* data, int size);

//
/// \brief converts type T to a std::string
/// \pre stringstream << T must be defined
/// \post None
/// \return a string representation of t
//
template <class T>
std::string tostr(const T& t)
{
  std::stringstream ss;
  ss << t;
  return ss.str();
}
