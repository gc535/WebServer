#
# Makefile
# web_server
#
# created by changkun at shiyanlou.com
#
 
CXX = g++
EXEC_HTTP = server_http
 
SOURCE_HTTP := main.cpp ServerBase.cpp
 
#OBJECTS_HTTP = main_http.o
 
# 开启编译器 O3 优化, pthread 启用多线程支持
LDFLAGS_COMMON = -std=c++11 -O3 -pthread -lboost_system
LDFLAGS_HTTP =
 
LPATH_COMMON = -I/usr/local/include/boost
LPATH_HTTP =
 
LLIB_COMMON = -L/usr/local/lib
LLIB_HTTP = 
 
http:
    $(CXX) $(SOURCE_HTTP) $(LDFLAGS_COMMON) $(LDFLAGS_HTTP) $(LPATH_COMMON) $(LPATH_HTTP) $(LLIB_COMMON) $(LLIB_HTTP) -o $(EXEC_HTTP)
 
clean:
    rm -f $(EXEC_HTTP) *.o
