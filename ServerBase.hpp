#pragma once

// stdlib
#include <iostream>
 
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
    void start()
    {
        // 默认资源放在 vector 的末尾, 用作默认应答
        // 默认的请求会在找不到匹配请求路径时，进行访问，故在最后添加
        for(auto it=resource.begin(); it!=resource.end();it++) 
        {
            all_resources.push_back(it);
        }
        for(auto it=default_resource.begin(); it!=default_resource.end();it++) 
        {
            all_resources.push_back(it);
        }
     
        // calling server specific accept function
        accept();
     
        // if more than 1 thread needed
        for(size_t c=1;c<num_threads;c++) 
        {
            // passing a lambda function to 
            // construct and run new thread
            // at the same time
            threads.emplace_back([this](){  
                m_io_service.run();
            });
        }
     
        // 主线程
        m_io_service.run();
     
        // 等待其他线程，如果有的话, 就等待这些线程的结束
        for(auto& t: threads)
        {
            t.join();
        }
        std::cout<<"exit start"<<std::endl;
    }

protected:
    // pure virtual, server-type specific function 
    virtual void accept()=0;

    // handle request and respond
    void process_request_and_respond(std::shared_ptr<socket_type> socket) const
    {
        // 为 async_read_untile() 创建新的读缓存
        // shared_ptr 用于传递临时对象给匿名函数
        // 会被推导为 std::shared_ptr<boost::asio::streambuf>
        auto read_buffer = std::make_shared<boost::asio::streambuf>();
     
        boost::asio::async_read_until(*socket, *read_buffer, "\r\n\r\n",
        [this, socket, read_buffer](const boost::system::error_code& ec, size_t bytes_transferred) 
        {
            if(!ec) 
            {
                // 注意：read_buffer->size() 的大小并一定和 bytes_transferred 相等， Boost 的文档中指出：
                // 在 async_read_until 操作成功后,  streambuf 在界定符之外可能包含一些额外的的数据
                // 所以较好的做法是直接从流中提取并解析当前 read_buffer 左边的报头, 再拼接 async_read 后面的内容
                size_t total = read_buffer->size();
     
                // 转换到 istream
                std::istream stream(read_buffer.get());
     
                // 接下来要将 stream 中的请求信息进行解析，然后保存到 request 对象中
                // 被推导为 std::shared_ptr<Request> 类型
                auto request = std::make_shared<Request>();
                *request = parse_request(stream);
     
                size_t num_additional_bytes = total-bytes_transferred;
     
                // 如果满足，同样读取
                if(request->header.count("Content-Length")>0) 
                {
                    boost::asio::async_read(*socket, *read_buffer,
                    boost::asio::transfer_exactly(stoull(request->header["Content-Length"]) - num_additional_bytes),
                    [this, socket, read_buffer, request](const boost::system::error_code& ec, size_t bytes_transferred) 
                    {
                        if(!ec) 
                        {
                            // 将指针作为 istream 对象存储到 read_buffer 中
                            request->content = std::shared_ptr<std::istream>(new std::istream(read_buffer.get()));
                            respond(socket, request);
                        }
                    });
                } 
                else 
                {
                    respond(socket, request);
                }
            }

        });
        std::cout<<"exit p_n_r"<<std::endl;
    }

    // response to the request
    void respond(std::shared_ptr<socket_type> socket, std::shared_ptr<Request> request) const
    {
        // repsond 
        for (auto res_it: all_resources)
        {
            std::regex e(res_it->first);
            std::smatch sm_res;
            // check if this object exist on the server by 
            // iterate thru all item in the map by the server
            // [TODO: this searching might be optimizable]
            if(std::regex_match(request->path, sm_res, e))
            {   
                // check if this object support this action
                if(res_it->second.count(request->method) > 0)
                {
                    request->path_match = move(sm_res);
                    auto write_buf = std::make_shared<boost::asio::streambuf>();
                    std::ostream response(write_buf.get());
                    // write the reponse to write buffer
                    res_it->second[request->method](response, *request);
                    // transfer data stored in write buffer back to client

                    boost::asio::async_write(*socket, *write_buf, 
                    [this, socket, request, write_buf](boost::system::error_code ec, size_t bytes_transferred)
                    {
                        // HTTPS (HTTP/1.1) permaient connection, calling recersively
                        if(!ec)
                            process_request_and_respond(socket);
                    });
                }
            }

        }
        std::cout<<"exit respond"<<std::endl;
    }


private:
    // parsing request message
    Request parse_request(std::istream& stream) const
    {
        Request request;
     
        // 使用正则表达式对请求报头进行解析，通过下面的正则表达式
        // 可以解析出请求方法(GET/POST)、请求路径以及 HTTP 版本
        std::regex e("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
     
        std::smatch sub_match;
     
        //从第一行中解析请求方法、路径和 HTTP 版本
        std::string line;
        getline(stream, line);
        line.pop_back();
        if(std::regex_match(line, sub_match, e)) 
        {
            request.method       = sub_match[1];
            request.path         = sub_match[2];
            request.http_version = sub_match[3];
     
            // 解析头部的其他信息
            bool matched;
            e="^([^:]*): ?(.*)$";
            do 
            {
                getline(stream, line);
                line.pop_back();
                matched=std::regex_match(line, sub_match, e);
                if(matched) 
                {
                    request.header[sub_match[1]] = sub_match[2];
                }
            } while(matched==true);
        }
        return request;
    }


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

