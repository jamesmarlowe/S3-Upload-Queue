//
/// \file responder.cpp
/// \author James Marlowe
/// \brief Implementation of responder functionality
//

#include "worker.h"
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
  std::string config_file = "";
  for (int i = 1; i < argc; i ++)
  {
    if (std::strcmp(argv[i], "-d") == 0)
      daemonize();
    if (strstr(argv[i],".conf"))
      config_file = argv[i];
  }
  
  google::SetLogDestination(google::INFO, LOG_DIR.c_str());
  google::InitGoogleLogging(argv[0]);
  
  google::InstallFailureSignalHandler();
  
  google::InstallFailureWriter(*write_stack_trace_to_log);
  
  config_file = (config_file=="")?"s3.conf":config_file;
  bson::Document s3conf = parse_config(config_file);
  if (s3conf.field_names().count("aws_id"))
    aws_id = s3conf["aws_id"].data<std::string>();
  if (s3conf.field_names().count("aws_secret"))
    aws_secret = s3conf["aws_secret"].data<std::string>();
  
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

CURLcode curl_get(const std::string& url, long timeout = 3)
{
    CURLcode code(CURLE_FAILED_INIT);
    CURL* curl = curl_easy_init();

    if(curl)
    {
        // set options for the request
        if(CURLE_OK == (code = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &data_write))
        && CURLE_OK == (code = curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L))
        && CURLE_OK == (code = curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L))
        && CURLE_OK == (code = curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout))
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


CURLcode curl_upload(const std::string& url, struct curl_slist *headerlist, const void *upload_body, long timeout = 3)
{
    CURLcode code(CURLE_FAILED_INIT);
    CURL* curl = curl_easy_init();

    if(curl)
    {
        // set options for the request
        if(CURLE_OK == (code = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &data_write))
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

void upload(S3Upload& req)
{
    curl_global_init(CURL_GLOBAL_ALL);
    req.set_success(false);
    
    struct curl_slist *m_headerlist = NULL;
    const char *post_body = NULL;
    
    time_t t = time(0);
    tm now = *gmtime(&t);
    char tmdescr[200]={0};
    const char fmt[]="Date: %a, %d %b %Y %X +0000"; 
    long unsigned int timestr = strftime(tmdescr, sizeof(tmdescr)-1, fmt, &now);
    std::string content_type = "binary/octet-stream";
    char StringToSign[230];
    char newline = 10;
    strcpy(StringToSign,"PUT");
    strcat(StringToSign, &newline);
    strcat(StringToSign, &newline);
    strcat(StringToSign, content_type.c_str());
    strcat(StringToSign, &newline);
    strcat(StringToSign, tmdescr);
    strcat(StringToSign, &newline);
    strcat(StringToSign, req.destination().c_str());

    unsigned char* digest = HMAC(EVP_sha1(), &aws_secret, aws_secret.length(), (unsigned char*)StringToSign, (sizeof(StringToSign) / sizeof(StringToSign[0])), NULL, NULL);

    std::string auth = "AWS "+aws_id+":"+base64_encode(digest, 20);
    m_headerlist = curl_slist_append(m_headerlist, tmdescr);
    m_headerlist = curl_slist_append(m_headerlist, ("Authorization: " + auth).c_str());
    m_headerlist = curl_slist_append(m_headerlist, ("content-type: " + content_type).c_str());
    m_headerlist = curl_slist_append(m_headerlist, "Content-MD5: ");
    
    if(req.has_upload_url() and (CURLE_OK == curl_get(req.upload_url())))
        post_body = response_data.c_str();
    else
        if (req.has_upload_content())
            post_body = req.upload_content().c_str();
        else
            return;
    
    if(CURLE_OK == curl_upload(req.destination(), m_headerlist, post_body))
    {
        req.set_success(true);
        std::cout << std::to_string(response_time) << std::endl;
        std::cout << std::to_string(response_code) << std::endl;
        std::cout << response_data << std::endl;
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
  zmqcpp::Socket socket(ZMQ_REP);
  zmqcpp::Message mesg;
  S3Upload dat;
  std::string annotated;
  socket.connect(ROUTER);
  while (true)
  {
    socket >> mesg;
    dat.ParseFromString(mesg.last());
    upload(dat);
    CHECK(dat.has_success() == true) << "Failed to return a response!";
    mesg.clear();
    dat.SerializeToString(& annotated);
    socket << zmqcpp::Message(annotated);
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
