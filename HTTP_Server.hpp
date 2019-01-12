#include <ServerBase.hpp>
#include <handler.hpp>

namespace WebFramework
{
template<>
class Server<HTTP>: public ServerBase<HTTP>
{
public:
Server(size_t port, size_t num_threads=1): 
    ServerBase<HTTP>(port, num_threads){}

protected: 
    virtual void accept() 
    {
        // adding a new awaiting-accept work on the io_service
        auto socket = std::make_shared<HTTP>(m_io_service);
        acceptor.async_accept(*socket, 
                             [this, socket](const boost::system::error_code& ec) 
        {
            // upon current sucessful connection, adding  
            // a new awaiting-accept work current connection
            accept();  

            // process the request on the current connection
            if(!ec) 
            {
                // Accept succeeded, maintain pernimant
                // connection by recursive call (HTTP/1.1)
                process_request_and_respond(socket);
            }
        });
    }


};

}
