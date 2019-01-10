#include <ServerBase.hpp>

namespace WebFramework
{
template<typename T>  // socket_type of the class
void ServerBase<T>::start() 
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
 
    // 调用 socket 的连接方式，还需要子类来实现 accept() 逻辑
    accept();
 
    // 如果 num_threads>1, 那么 m_io_service.run()
    // 将运行 (num_threads-1) 线程成为线程池
    // create thread pool
    for(size_t c=1;c<num_threads;c++) 
    {
        threads.emplace_back([this](){  // lambda function
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
}


// 处理请求和应答
template<typename T>  // socket_type of the class
void ServerBase<T>::process_request_and_respond(std::shared_ptr<T> socket) const 
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
}

// respond
// TODO:    
template<typename T>  // socket_type of the class   
void ServerBase<T>::respond(std::shared_ptr<T> socket, std::shared_ptr<Request> request) const 
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
}


// 解析请求
template<typename T>  // socket_type of the class 
Request ServerBase<T>::parse_request(std::istream& stream) const 
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


}