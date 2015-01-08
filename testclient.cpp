//
/// \file testclient.cpp
/// \author James Marlowe
/// \brief A simple test to call the router and print the response
//
#include <zmq.hpp>
#include <string>
#include <iostream>
#include "s3_upload.pb.h"

using namespace std;

int main (int argc, char* argv[])
{
    //  Prepare our context and socket
    zmq::context_t context (1);
    zmq::socket_t socket (context, ZMQ_REQ);
    std::cout << "Connecting to annotation server…" << std::endl;
    socket.connect ("tcp://localhost:5555");

    int reqs = 2;
    if (argc > 1)
        reqs = atoi(argv[1]);
    
    S3Upload data, data2;
    data.set_upload_url("http://lorempixel.com/400/200/");
    data.set_destination("/customer_campaigns/iurls/test");
    
    std::cout << "Sending requests…" << std::endl;
    for (int i=0; i<reqs; i++)
    {
        zmq::message_t msg(data.ByteSize());
        data.SerializeToArray(msg.data(), data.GetCachedSize());
        socket.send(msg);
        zmq::message_t rsp;
        socket.recv(&rsp);
        data2.ParseFromArray(rsp.data(), rsp.size());
    }

    cout << data2.success() << endl;
    cout << data2.destination() << endl;
    google::protobuf::ShutdownProtobufLibrary();
    return 0;
}
