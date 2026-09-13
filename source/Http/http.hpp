#pragma once
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <regex>
#include <sys/stat.h>
#include "../Reactor/Reactor.hpp"

#define DEFALT_TIMEOUT 30

std::unordered_map<int, std::string> _statu_msg = {
    {100, "Continue"},
    {101, "Switching Protocol"},
    {102, "Processing"},
    {103, "Early Hints"},
    {200, "OK"},
    {201, "Created"},
    {202, "Accepted"},
    {203, "Non-Authoritative Information"},
    {204, "No Content"},
    {205, "Reset Content"},
    {206, "Partial Content"},
    {207, "Multi-Status"},
    {208, "Already Reported"},
    {226, "IM Used"},
    {300, "Multiple Choice"},
    {301, "Moved Permanently"},
    {302, "Found"},
    {303, "See Other"},
    {304, "Not Modified"},
    {305, "Use Proxy"},
    {306, "unused"},
    {307, "Temporary Redirect"},
    {308, "Permanent Redirect"},
    {400, "Bad Request"},
    {401, "Unauthorized"},
    {402, "Payment Required"},
    {403, "Forbidden"},
    {404, "Not Found"},
    {405, "Method Not Allowed"},
    {406, "Not Acceptable"},
    {407, "Proxy Authentication Required"},
    {408, "Request Timeout"},
    {409, "Conflict"},
    {410, "Gone"},
    {411, "Length Required"},
    {412, "Precondition Failed"},
    {413, "Payload Too Large"},
    {414, "URI Too Long"},
    {415, "Unsupported Media Type"},
    {416, "Range Not Satisfiable"},
    {417, "Expectation Failed"},
    {418, "I'm a teapot"},
    {421, "Misdirected Request"},
    {422, "Unprocessable Entity"},
    {423, "Locked"},
    {424, "Failed Dependency"},
    {425, "Too Early"},
    {426, "Upgrade Required"},
    {428, "Precondition Required"},
    {429, "Too Many Requests"},
    {431, "Request Header Fields Too Large"},
    {451, "Unavailable For Legal Reasons"},
    {501, "Not Implemented"},
    {502, "Bad Gateway"},
    {503, "Service Unavailable"},
    {504, "Gateway Timeout"},
    {505, "HTTP Version Not Supported"},
    {506, "Variant Also Negotiates"},
    {507, "Insufficient Storage"},
    {508, "Loop Detected"},
    {510, "Not Extended"},
    {511, "Network Authentication Required"}};

std::unordered_map<std::string, std::string> _mime_msg = {
    {".aac", "audio/aac"},
    {".abw", "application/x-abiword"},
    {".arc", "application/x-freearc"},
    {".avi", "video/x-msvideo"},
    {".azw", "application/vnd.amazon.ebook"},
    {".bin", "application/octet-stream"},
    {".bmp", "image/bmp"},
    {".bz", "application/x-bzip"},
    {".bz2", "application/x-bzip2"},
    {".csh", "application/x-csh"},
    {".css", "text/css"},
    {".csv", "text/csv"},
    {".doc", "application/msword"},
    {".docx", "application/vnd.openxmlformats-officedocument.wordprocessingml.document"},
    {".eot", "application/vnd.ms-fontobject"},
    {".epub", "application/epub+zip"},
    {".gif", "image/gif"},
    {".htm", "text/html"},
    {".html", "text/html"},
    {".ico", "image/vnd.microsoft.icon"},
    {".ics", "text/calendar"},
    {".jar", "application/java-archive"},
    {".jpeg", "image/jpeg"},
    {".jpg", "image/jpeg"},
    {".js", "text/javascript"},
    {".json", "application/json"},
    {".jsonld", "application/ld+json"},
    {".mid", "audio/midi"},
    {".midi", "audio/x-midi"},
    {".mjs", "text/javascript"},
    {".mp3", "audio/mpeg"},
    {".mpeg", "video/mpeg"},
    {".mpkg", "application/vnd.apple.installer+xml"},
    {".odp", "application/vnd.oasis.opendocument.presentation"},
    {".ods", "application/vnd.oasis.opendocument.spreadsheet"},
    {".odt", "application/vnd.oasis.opendocument.text"},
    {".oga", "audio/ogg"},
    {".ogv", "video/ogg"},
    {".ogx", "application/ogg"},
    {".otf", "font/otf"},
    {".png", "image/png"},
    {".pdf", "application/pdf"},
    {".ppt", "application/vnd.ms-powerpoint"},
    {".pptx", "application/vnd.openxmlformats-officedocument.presentationml.presentation"},
    {".rar", "application/x-rar-compressed"},
    {".rtf", "application/rtf"},
    {".sh", "application/x-sh"},
    {".svg", "image/svg+xml"},
    {".swf", "application/x-shockwave-flash"},
    {".tar", "application/x-tar"},
    {".tif", "image/tiff"},
    {".tiff", "image/tiff"},
    {".ttf", "font/ttf"},
    {".txt", "text/plain"},
    {".vsd", "application/vnd.visio"},
    {".wav", "audio/wav"},
    {".weba", "audio/webm"},
    {".webm", "video/webm"},
    {".webp", "image/webp"},
    {".woff", "font/woff"},
    {".woff2", "font/woff2"},
    {".xhtml", "application/xhtml+xml"},
    {".xls", "application/vnd.ms-excel"},
    {".xlsx", "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"},
    {".xml", "application/xml"},
    {".xul", "application/vnd.mozilla.xul+xml"},
    {".zip", "application/zip"},
    {".3gp", "video/3gpp"},
    {".3g2", "video/3gpp2"},
    {".7z", "application/x-7z-compressed"}};

// 封装配置操作
class Util
{
public:
    // 批量分割字符串操作,返回分割成子字符串的个数
    static size_t Split(const std::string &src, const std::string &sep, std::vector<std::string> *arry)
    {
        // eg:a,,b,c, sep=','
        size_t offset = 0; // 分割的起始位置(偏移量)
        while (src.size() < offset)
        {
            size_t pos = src.find(sep, offset);
            if (pos == std::string::npos) // 找不到的情况，把剩余部分打包成一个子串返回
            {
                if (offset == arry->size()) // 起始位置在末尾
                    break;
                arry->push_back(src.substr(offset)); // 偏移位置到末尾
                break;
            }
            if (pos == offset) // 连续分隔符的问题，跳过空串
            {
                offset = pos + sep.size();
                continue;
            }
            arry->push_back(src.substr(offset, pos - offset)); // 从偏移量到分隔符前
            offset = pos + sep.size();
        }
        return arry->size();
    }

    // 读取文件所有内容操作,放入栈区
    static bool ReadFile(const std::string &filename, std::string *buf)
    {
        std::ifstream ifs(filename, std::ios::binary); // 以二进制打开，防止特殊字符被转义
        if (ifs.is_open() == false)
        {
            LOG(LOGLEVEL::ERR, "Open File:%s Failed!!!", filename.c_str());
            return false;
        }

        size_t fsize = 0;      // 文件大小
        ifs.seekg(0, ifs.end); // 将光标移动到文件末尾
        fsize = ifs.tellg();   // end到文件起始处正好是文件大小
        ifs.seekg(0, ifs.beg); // 重新移动光标到开头处

        // 扩容
        buf->resize(fsize);
        ifs.read(buf->data(), fsize); // c_str()返回的是const char*，data()返回的是char *,即可以被修改

        if (ifs.good() == false)
        {
            LOG(LOGLEVEL::ERR, "Read File:%s Failed!!!", filename.c_str());
            ifs.close();
            return false;
        }
        ifs.close();
        return true;
    }

    // 文件写入操作
    static bool WriteFile(const std::string &filename, const std::string &buf)
    {
        std::ofstream ofs(filename, std::ios::binary | std::ios::trunc); // trunc截断清空数据
        if (ofs.is_open() == false)
        {
            LOG(LOGLEVEL::ERR, "Open File:%s Failed!!!", filename.c_str());
            return false;
        }

        ofs.write(buf.c_str(), buf.size()); // c_str()返回的是const char*，data()返回的是char *,即可以被修改

        if (ofs.good() == false)
        {
            LOG(LOGLEVEL::ERR, "Write File:%s Failed!!!", filename.c_str());
            ofs.close();
            return false;
        }
        ofs.close();
        return true;
    }

    // 编码格式：将特殊字符的ascii值，转换为两个16进制字符，前缀%   C++ -> C%2B%2B
    // 不编码的特殊字符： RFC3986文档规定 . - _ ~ 字母，数字属于绝对不编码字符
    // RFC3986文档规定，编码格式 %HH
    // W3C标准中规定，查询字符串中的空格，需要编码为+， 解码则是+转空格
    // url编码操作
    static std::string UrlEncode(const std::string url, bool convert_space_to_plus)
    {
        std::string res;
        for (auto &c : url)
        {
            if (c == '.' || c == '-' || c == '_' || c == '~' || isalnum(c)) // 特殊字符不编码
            {
                res += c;
                continue;
            }
            if (c == ' ' && convert_space_to_plus == true) // 有开空格转加号则按W3C标准来
            {
                res += '+';
                continue;
            }
            char tmp[4] = {0}; // 其余字符均需要编码,格式为%HH,%%输出为%
            snprintf(tmp, 4, "%%%02X", c);
            res += tmp;
        }
        return res;
    }

    // 十六进制转整数操作
    static char HEXTOI(char c)
    {
        if (c >= '0' && c <= '9')
        {
            return c - '0';
        }
        else if (c >= 'a' && c <= 'f') // 十六进制a表示10
        {
            return c - 'a' + 10;
        }
        else if (c >= 'A' && c <= 'F')
        {
            return c - 'A' + 10;
        }
        else
        {
            return -1; // 非十六进制数字
        }
    }

    // url解码操作
    static std::string UrlDecode(const std::string url, bool convert_plus_to_space)
    {
        std::string res;
        for (int i = 0; i < url.size(); i++)
        {
            const char c = url[i];
            if (c == '+' && convert_plus_to_space == true) // 有开加号转空格则按W3C标准来
            {
                res += ' ';
                continue;
            }
            if (c == '%' && (i + 2) < url.size()) // 其余字符均需要解码,编码格式为%HH
            {
                char v1 = HEXTOI(url[i + 1]);
                char v2 = HEXTOI(url[i + 2]);
                char v = v1 * 16 + v2; // 算回原来的数字
                res += v;
                i += 2; // 跳转到下一个需要解码的字符
                continue;
            }
            res += c;
        }
        return res;
    }

    // 响应状态码的描述信息获取操作
    static std::string StatuDesc(int statu)
    {
        auto it = _statu_msg.find(statu);
        if (it == _statu_msg.end())
        {
            return "Unknown";
        }
        else
        {
            return it->second;
        }
    }

    // 根据文件后缀名获取文件mime
    static std::string ExtMime(const std::string &filename)
    {
        size_t pos = filename.find_last_of('.');
        if (pos == std::string::npos)
        {
            return "application/octet-stream"; // 默认二进制流
        }

        std::string ext = filename.substr(pos);
        auto it = _mime_msg.find(ext);
        if (it == _mime_msg.end())
        {
            return "application/octet-stream"; // 默认二进制流
        }
        return it->second;
    }
};