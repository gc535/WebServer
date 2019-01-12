#include <ServerBase.hpp>
#include <fstream>
#include <string>

namespace
{

template<>
class Server<HTTPS>: public ServerBase<HTTPS>
{
public:
    Server(size_t port, size_t num_threads, 
           const std::string& cert, const std::string& key):
        ServerBase(port, num_threads), 
        context(boost::asio::ssl::context::sslv23)
        {
            // init key and cert file
            // TODO

        }

protected:
    virtual accpet()
    {
        // make tmp pointer of the socket
        auto socket = std::make_shared<HTTPS>(io_service, context);

        acceptor.async_accept(*socket, 
                              [this](const boost::system::error_code& ec)
        {
            accept();

            if(!ec)
            {
                // perform handshake upon connection
                socket->async_handshake(boost::asio::ssl::stream_base::server,  
                                        [this](const boost::system::error_code& ec)
                {
                    // if handshake successful
                    if(!ec)
                        process_request_and_respond(socket);
                    
                };)
            }

        };)
    }

protected:
    boost::asio::ssl::context context;
    


};

}