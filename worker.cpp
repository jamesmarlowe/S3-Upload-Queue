//
/// \file responder.cpp
/// \author James Marlowe
/// \brief Implementation of responder functionality
//

#include "responder.h"
#include <vector>
#include <thread>
#include <glog/logging.h>

using namespace std;

const std::string LOG_DIR = "/var/log/server/";
std::string response_data;
long response_code;
double response_time;
std::string aws_id;
std::string aws_secret;

int main(int argc, char* argv[])
{
  for (int i = 1; i < argc; i ++)
  {
    if (std::strcmp(argv[i], "-d") == 0)
      daemonize();
  }
  
  google::SetLogDestination(google::INFO, LOG_DIR.c_str());
  google::InitGoogleLogging(argv[0]);
  
  google::InstallFailureSignalHandler();
  
  google::InstallFailureWriter(*write_stack_trace_to_log);
  
  responder();
  return 0;
}

static size_t data_write(char* buf, size_t size, size_t nmemb, void* userp)
{
    for (int c = 0; c<size*nmemb; c++)
    {
        response_data.push_back(buf[c]);
    }
    return size*nmemb;
}

static size_t read_callback(void *ptr, size_t size, size_t nmemb, void *stream)
{

  size_t retcode = "test";

  return retcode;
}

CURLcode curl_read(const std::string& url, struct curl_slist *headerlist, void *upload_body, long timeout = 3)
{
    CURLcode code(CURLE_FAILED_INIT);
    CURL* curl = curl_easy_init();

    if(curl)
    {
        // set options for the request
        if(CURLE_OK == (code = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &data_write))
        && CURLE_OK == (code = curl_easy_setopt(curl, CURLOPT_READFUNCTION, &read_callback))
        && CURLE_OK == (code = curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L))
        && CURLE_OK == (code = curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L))
        && CURLE_OK == (code = curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout))
        && CURLE_OK == (code = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist))
        && CURLE_OK == (code = curl_easy_setopt(curl, CURLOPT_READDATA, upload_body))
        && CURLE_OK == (code = curl_easy_setopt(curl, CURLOPT_URL, url.c_str())))
        {
            code = curl_easy_perform(curl);
        }
        // get stats on the response
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
        curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &response_time);
        
        curl_easy_cleanup(curl);
    }
    return code;
}


{

}

void upload(S3Upload& req)
{
    curl_global_init(CURL_GLOBAL_ALL);
    req.set_success(false);
    
    struct curl_slist *m_headerlist = NULL;
    const char *post_body = NULL;
    
    time_t now = time(0);
    std::string content_type = "binary/octet-stream";
    std::string StringToSign = "PUT"+char(10)+char(10)+content_type+char(10)+now+char(10)+req.destination()

    unsigned char* digest = HMAC(EVP_sha1(), aws_secret, aws_secret.length(), (unsigned char*)StringToSign, StringToSign.length(), NULL, NULL);    
    
    std::string auth = "AWS "+aws_id+":"+digest;
    m_headerlist = curl_slist_append(m_headerlist, "Date: " + now);
    m_headerlist = curl_slist_append(m_headerlist, "Authorization: " + auth);
    m_headerlist = curl_slist_append(m_headerlist, "content-type: " + content_type);
    m_headerlist = curl_slist_append(m_headerlist, "Content-MD5: ");
    
    if(req.has_request_url())
    {
        if(CURLE_OK == curl_read(req.destination(), m_headerlist, post_body))
        {
            req.set_success(true);
            req.set_response_time(std::to_string(response_time));
            req.set_response_status(std::to_string(response_code));
            req.set_response_body(response_data);
        }
    }

    curl_slist_free_all(m_headerlist); 
    curl_global_cleanup();
    response_data.clear();
    response_code = 0;
    response_time = 0;
    return;
}

void responder ()
{
  zmqcpp::Socket socket(ZMQ_PULL);
  zmqcpp::Message mesg;
  S3Upload dat;
  std::string annotated;
  socket.connect(ROUTER);
  while (true)
  {
    socket >> mesg;
    dat.ParseFromString(mesg.last());
    annotate_request(dat);
    CHECK(dat.has_success() == true) << "Failed to return a response!";
    mesg.clear();
  }
  return;
}

void daemonize()
{
  pid_t pid;
  pid = fork();
  if (pid < 0)
    exit(EXIT_FAILURE);
  if (pid > 0)
    exit(EXIT_SUCCESS);
}

void write_stack_trace_to_log(const char* data, int size)
{
  std::ofstream log_file;
  std::string lfile = LOG_DIR + "stacktrace.log";
  log_file.open(lfile.c_str(), std::ios::out | std::ios::app);
  log_file << data;
  log_file.close();
  
  return;
}
