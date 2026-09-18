#include <iostream>
#include <regex>
#include <string>

int main()
{
    std::string abc("GET /index.html?user=root&passwd=123 HTTP/1.1");
    std::smatch matches;                                                        // 匹配到的结果都会放进matches里
    std::regex e("(GET|HEAD|PUT|DELETE) ([^?]*)(?:\\?(.*))? (HTTP/1\\.[01])?"); // 正则
    std::regex_match(abc, matches, e);
    std::cout << matches[0] << std::endl;
    std::cout << "method: " << matches[1] << std::endl;
    std::cout << "uri: " << matches[2] << std::endl;
    std::cout << "params: " << matches[3] << std::endl;
    std::cout << "version: " << matches[4] << std::endl;
    return 0;
}