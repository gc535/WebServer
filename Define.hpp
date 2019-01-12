#pragma once

/* defines datatype and struct used by the web framework */
namespace WebFramework 
{

// define server types
typedef boost::asio::ip::tcp::socket HTTP;
typedef boost::asio::ssl::stream<boost::asio::ip::tcp::socket> HTTPS;

// Request struct contains the request header, data etc.
struct Request 
{
    // method: (POST/GET); path; HTTP version
    std::string method, path, http_version;
    // refcont the content
    std::shared_ptr<std::istream> content;
    // stores the header
    std::unordered_map<std::string, std::string> header;
    // reg match the path info
    std::smatch path_match;
};


// define resource: string -> map(string -> handling function)
/*
    Resource stores functions that handles the user request by case:
        path: method : function
        path: is the a specific object that is been requested. (xxx.html)
        method: is the action that user want to execute on this object:
                        post: edit something on .html
                        get: get something from .html
                        ...
        function: is the function that handle this specific action on the server.    
 */
typedef std::map<std::string, std::unordered_map<std::string,
                 std::function<void(std::ostream&, Request&)>>> resource_type;

}
