#pragma once
 
// stl
#include <unordered_map>

// boost
#include <regex>
#include <thread>
#include <boost/asio.hpp>

// local
#include <Define.hpp> 


namespace WebFramework 
{
// socket_type can be HTTP or HTTPS
template <typename socket_type>
class ServerBase {
public:
    // 构造服务器, 初始化端口, 默认使用一个线程
    ServerBase(unsigned short port, size_t num_threads=1) :
        endpoint(boost::asio::ip::tcp::v4(), port),
        acceptor(m_io_service, endpoint),
        num_threads(num_threads) {}

    // start the server
    void start();

protected:
    // pure virtual, server-type specific function 
    virtual void accept()=0;

    // handle request and respond
    void process_request_and_respond(std::shared_ptr<socket_type> socket) const;

    // response to the request
    void respond(std::shared_ptr<socket_type> socket, std::shared_ptr<Request> request) const;
private:
    // parsing request message
    Request parse_request(std::istream& stream) const;


public:
    // stores regular resources
    resource_type resource;
    // stores default resources
    resource_type default_resource;

protected:
    // asio 库中的 io_service 是调度器，所有的异步 IO 事件都要通过它来分发处理
    // 换句话说, 需要 IO 的对象的构造函数，都需要传入一个 io_service 对象
    boost::asio::io_service m_io_service;
    // IP 地址、端口号、协议版本构成一个 endpoint，并通过这个 endpoint 在服务端生成
    // tcp::acceptor 对象，并在指定端口上等待连接
    boost::asio::ip::tcp::endpoint endpoint;
    // 所以，一个 acceptor 对象的构造都需要 io_service 和 endpoint 两个参数
    boost::asio::ip::tcp::acceptor acceptor;

    // 服务器线程
    size_t num_threads;
    std::vector<std::thread> threads;

    // for handling resources
    std::vector<resource_type::iterator> all_resources;
    
};

template<typename socket_type>
class Server : public ServerBase<socket_type> {};


}

