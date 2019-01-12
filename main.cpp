#include <HTTP_Server.hpp>
#include <handler.hpp>

int main()
{   
    WebFramework::Server<WebFramework::HTTP> http_server(12345, 1);
    WebFramework::start_server<WebFramework::Server<WebFramework::HTTP> >(http_server);


    return 0;
}