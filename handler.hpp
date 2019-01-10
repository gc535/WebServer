#pragma once 

#include <Define.hpp>
#include <ServerBase.hpp>


template<typename SERVER_TYPE>
void start_server(SERVER_TYPE& server)
{
    // define server resources



    server.resource["^/info/?$"]["GET"] = [](std::ostream& response, 
                                             Request& request)
    {

    }


    // start the server
    server.start();
}