#include <iostream>
#include <memory>
#include "../source/Http/http.hpp"

void Usage(std::string proc)
{
    std::cerr << "Usage: " << proc << " server_ip server_port" << std::endl;
}

// 登录方法
void Login(const HttpRequest &req, HttpResponse *resp)
{
    std::string text = "hello world";
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        Usage(argv[0]);
        exit(-1);
    }

    uint16_t port = std::stoi(argv[1]);
    std::unique_ptr<HttpServer> svr = std::make_unique<HttpServer>(port); // 创建Http版块
    svr->SetThreadCount(4);
    svr->SetBaseDir("../source/Http/wwwroot");
    svr->RegisterGet("login.html", Login);
    svr->Listen();

    return 0;
}
