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

const std::string default_basedir = "/home/yzq/Flux/source/Http/wwwroot"; // 默认静态资源的地址

// 封装配置操作
class Util
{
public:
    // 批量分割字符串操作,返回分割成子字符串的个数
    static size_t Split(const std::string &src, const std::string &sep, std::vector<std::string> *arry)
    {
        // eg:a,,b,c, sep=','
        size_t offset = 0; // 分割的起始位置(偏移量)
        while (offset < src.size())
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

    // 判断一个文件是否是一个目录操作
    static bool IsDirectory(const std::string &filename)
    {
        struct stat st;
        int n = stat(filename.c_str(), &st);
        if (n < 0)
        {
            LOG(LOGLEVEL::WARNING, "Error in determining file:%s type!!!", filename.c_str());
            return false;
        }
        return S_ISDIR(st.st_mode);
    }

    // 判断一个文件是否是一个普通文件操作
    static bool IsRegular(const std::string &filename)
    {
        struct stat st;
        int n = stat(filename.c_str(), &st);
        if (n < 0)
        {
            LOG(LOGLEVEL::WARNING, "Error in determining file:%s type!!!", filename.c_str());
            return false;
        }
        return S_ISREG(st.st_mode);
    }

    // 判断资源请求路径是否合法
    static bool ValidPath(const std::string &path)
    {
        // 按照'/'分割，如果越级过根目录则非法
        std::vector<std::string> subdir;
        Split(path, "/", &subdir);

        int level = 0;
        for (auto &dir : subdir)
        {
            if (dir == "..")
            {
                level--;
                if (level < 0)
                    return false;
            }
            else
            {
                level++;
            }
        }
        return true;
    }
};

class HttpRequest
{
public:
    HttpRequest()
        : _version("HTTP/1.1")
    {
    }

    // 重置所有信息
    void Reset()
    {
        _method.clear();
        _path.clear();
        _version = "HTTP/1.1"; // 默认1.1版本
        _body.clear();
        std::smatch match; // 与新对象交换空间达到清空的效果
        _matches.swap(match);
        _headers.clear();
        _params.clear();
    }

    // 设置请求头字段
    void SetHeader(const std::string &key, const std::string &value)
    {
        _headers.insert(std::make_pair(key, value));
    }

    // 判断是否存在某个请求头字段
    bool HasHeader(const std::string &key) const
    {
        auto it = _headers.find(key);
        if (it != _headers.end())
        {
            return true;
        }
        return false;
    }

    // 获取某个请求头字段
    std::string GetHeader(const std::string &key) const
    {
        auto it = _headers.find(key);
        if (it == _headers.end())
        {
            return "";
        }
        return it->second;
    }

    // 插入查询字符串
    void SetParam(const std::string &key, const std::string &value)
    {
        _params.insert(std::make_pair(key, value));
    }

    // 判断是否有某个指定的查询字符串
    bool HasParam(const std::string &key) const
    {
        auto it = _params.find(key);
        if (it != _params.end())
        {
            return true;
        }
        return false;
    }

    // 获取指定的查询字符串
    std::string GetParam(const std::string &key) const
    {
        auto it = _params.find(key);
        if (it == _params.end())
        {
            return "";
        }
        return it->second;
    }

    // 获取正文长度
    size_t ContentLength() const
    {
        bool res = HasHeader("Content-Length");
        if (res == false)
        {
            return 0;
        }
        std::string len = GetHeader("Content-Length");
        return std::stol(len);
    }

    // 判断是否是短链接
    bool IsShortConnection() const
    {
        // 长连接的判定标准
        if (HasHeader("Connection") && GetHeader("Connection") == "keep-alive")
        {
            return false;
        }
        return true;
    }

public:
    std::string _method;                                   // 请求方法
    std::string _path;                                     // 请求路径
    std::string _version;                                  // 协议版本
    std::string _body;                                     // 请求体
    std::smatch _matches;                                  // 资源路径的正则
    std::unordered_map<std::string, std::string> _headers; // 请求头
    std::unordered_map<std::string, std::string> _params;  // 查询字符串(即uri?后的参数)
};

class HttpResponse
{
public:
    HttpResponse()
        : _status(200), _redirect_flag(false)
    {
    }

    HttpResponse(int status)
        : _status(status), _redirect_flag(false)
    {
    }

    void Reset()
    {
        _status = 200;
        _redirect_flag = false;
        _body.clear();
        _redirect_url.clear();
        _headers.clear();
    }

    void SetHeader(const std::string &key, const std::string &value)
    {
        _headers.insert(std::make_pair(key, value));
    }

    bool HasHeader(const std::string &key) const
    {
        auto it = _headers.find(key);
        if (it != _headers.end())
        {
            return true;
        }
        return false;
    }

    std::string GetHeader(const std::string &key) const
    {
        auto it = _headers.find(key);
        if (it == _headers.end())
        {
            return "";
        }
        return it->second;
    }

    void SetContent(const std::string &body, const std::string &type = "text/html")
    {
        _body = body;
        SetHeader("Content-Type", type);
    }

    // 开启重定向
    void EnableRedirect()
    {
        _redirect_flag = true;
    }

    // 设置重定向(默认临时重定向，永久重定向为301)
    void SetRedirect(const std::string &url, int status = 302)
    {
        if (_redirect_flag == false)
            return;
        _status = status;
        _redirect_url = url;
    }

    bool IsShortConnection() const
    {
        // 长连接的判定标准
        if (HasHeader("Connection") && GetHeader("Connection") == "keep-alive")
        {
            return false;
        }
        return true;
    }

public:
    int _status;                                           // 状态码
    bool _redirect_flag;                                   // 是否重定向，默认false
    std::string _body;                                     // 响应体
    std::string _redirect_url;                             // 重定向的url
    std::unordered_map<std::string, std::string> _headers; // 响应头
};

// Http请求数据所处的处理状态
typedef enum
{
    RECV_HTTP_ERROR = 0,
    RECV_HTTP_LINE, // 请求行
    RECV_HTTP_HEAD, // 请求头
    RECV_HTTP_BODY, // 请求体
    RECV_HTTP_OVER  // 请求接收完成
} HttpRecvStatus;

enum class ParseState // 解析请求行的状态
{
    NEED_MORE = 0, // 数据不够
    SUCCESS,       // 成功
    ERROR          // 错误
};

#define MAX_LINE 8192 // 一行最大的数据量

// Http的解析处理
class HttpContext
{
    // 解析请求行(GET /index.html?user=root&passwd=123 HTTP/1.1)
    bool ParseHttpLine(const std::string &line)
    {
        std::smatch matches;
        std::regex e("(GET|HEAD|POST|PUT|DELETE) ([^?]*)(?:\\?(.*))? (HTTP/1\\.[01])(?:\r\n|\n)?",
                     std::regex::icase);
        bool ret = std::regex_match(line, matches, e);
        if (ret == false)
        {
            _resp_status = 400;
            _recv_status = RECV_HTTP_ERROR;
            LOG(LOGLEVEL::ERR, "Parse HttpLine Failed!!!");
            return false;
        }
        // 0 : GET /index.html?user=root&passwd=123 HTTP/1.1
        // 1 : GET
        // 2 : /index.html
        // 3 : user=root&passwd=123
        // 4 : HTTP/1.1
        _request._method = matches[1]; // 请求方法
        std::transform(_request._method.begin(), _request._method.end(),
                       _request._method.begin(), ::toupper); // 转换为大写
        _request._path = Util::UrlDecode(matches[2], false); // 请求资源路径
        std::vector<std::string> query_string_arry;          // 查询字符串的键值对
        std::string query_string = matches[3];
        if (!query_string.empty()) // 非空才进行分割
        {
            Util::Split(query_string, "&", &query_string_arry); // 分割查询字符串
            for (auto &str : query_string_arry)
            {
                auto pos = str.find("=");
                if (pos == std::string::npos)
                {
                    _request.Reset(); // 重置所有信息
                    _resp_status = 400;
                    _recv_status = RECV_HTTP_ERROR;
                    LOG(LOGLEVEL::ERR, "Parse HttpLine Failed!!!");
                    return false;
                }
                std::string key = Util::UrlDecode(str.substr(0, pos), true);
                std::string val = Util::UrlDecode(str.substr(pos + 1), true);
                _request.SetParam(key, val); // 构建查询字符串
            }
        }
        _request._version = matches[4]; // 版本号
        return true;
    }

    // 接收请求行
    ParseState RecvHttpLine(Buffer &buf)
    {
        if (_recv_status != RECV_HTTP_LINE)
        {
            return ParseState::ERROR;
        }
        std::string line = buf.GetLineAndPop(); // 获取一行数据,不够一行数据返回空串
        if (line.size() == 0)
        {
            if (buf.ReadAbleSize() > MAX_LINE) // 没有换行符导致一直读
            {
                _resp_status = 414;
                _recv_status = RECV_HTTP_ERROR;
                return ParseState::ERROR;
            }
            return ParseState::NEED_MORE; // 数据不够，继续外部调用读取
        }
        // 到这里一定读到了一行数据
        if (line.size() > MAX_LINE)
        {
            _resp_status = 414;
            _recv_status = RECV_HTTP_ERROR;
            LOG(LOGLEVEL::WARNING, "HttpLine Too Long!!!");
            return ParseState::ERROR;
        }
        if (!ParseHttpLine(line)) // 解析失败
            return ParseState::ERROR;
        _recv_status = RECV_HTTP_HEAD; // 准备进入请求头处理状态
        return ParseState::SUCCESS;
    }

    // 解析请求头
    bool ParseHttpHead(std::string &line)
    {
        // 去掉换行符
        if (line.back() == '\n')
            line.pop_back();
        if (line.back() == '\r')
            line.pop_back();
        std::string symbol = ": "; // 请求头的键值对为key: value
        auto pos = line.find(symbol);
        if (pos == std::string::npos)
        {
            _resp_status = 400;
            _recv_status = RECV_HTTP_ERROR;
            LOG(LOGLEVEL::ERR, "Parse HttpHead Failed!!!");
            return false;
        }
        std::string key = line.substr(0, pos);
        std::string val = line.substr(pos + symbol.size());
        _request.SetHeader(key, val);
        return true;
    }

    // 接收请求头
    ParseState RecvHttpHead(Buffer &buf)
    {
        if (_recv_status != RECV_HTTP_HEAD)
        {
            return ParseState::ERROR;
        }

        // 循环读取，直到读到空行
        while (true)
        {
            std::string line = buf.GetLineAndPop(); // 获取一行数据,不够一行数据返回空串
            if (line.size() == 0)
            {
                if (buf.ReadAbleSize() > MAX_LINE) // 没有换行符导致一直读
                {
                    _resp_status = 414;
                    _recv_status = RECV_HTTP_ERROR;
                    return ParseState::ERROR;
                }
                return ParseState::NEED_MORE; // 数据不够，继续外部调用读取
            }
            // 到这里一定读到了一行数据
            if (line.size() > MAX_LINE)
            {
                _resp_status = 414;
                _recv_status = RECV_HTTP_ERROR;
                LOG(LOGLEVEL::WARNING, "HttpLine Too Long!!!");
                return ParseState::ERROR;
            }
            if (line == "\r\n" || line == "\n") // 读取到了空行
            {
                LOG(LOGLEVEL::INFO, "Parse HttpHead Finish!!!");
                break;
            }
            if (!ParseHttpHead(line)) // 解析失败
                return ParseState::ERROR;
        }
        _recv_status = RECV_HTTP_BODY; // 准备进入处理请求体状态
        return ParseState::SUCCESS;
    }

    // 接收请求体
    ParseState RecvHttpBody(Buffer &buf)
    {
        if (_recv_status != RECV_HTTP_BODY)
        {
            return ParseState::ERROR;
        }

        size_t content_len = _request.ContentLength();
        if (_request._body.size() > content_len)
        {
            return ParseState::ERROR;
        }
        if (content_len == 0)
        {
            _recv_status = RECV_HTTP_OVER;
            return ParseState::SUCCESS;
        }

        size_t need_len = content_len - _request._body.size(); // 实际还需要读取的长度
        if (need_len == 0)
        {
            _recv_status = RECV_HTTP_OVER;
            return ParseState::SUCCESS;
        }

        size_t readable = buf.ReadAbleSize();           // 真实可读的数据大小
        size_t take_len = std::min(need_len, readable); // 实际取出的长度
        _request._body.append(buf.ReaderPosition(), take_len);
        buf.MoveReadOffset(take_len);
        if (take_len == need_len)
        {
            _recv_status = RECV_HTTP_OVER;
            return ParseState::SUCCESS;
        }

        return ParseState::NEED_MORE;
    }

public:
    HttpContext()
        : _resp_status(200), _recv_status(RECV_HTTP_LINE)
    {
    }

    void Reset()
    {
        _resp_status = 200;
        _recv_status = RECV_HTTP_LINE;
        _request.Reset();
    }

    // 响应状态码
    int RespStatus()
    {
        return _resp_status;
    }

    // 请求接收状态
    HttpRecvStatus RecvStatus()
    {
        return _recv_status;
    }

    // Http请求
    HttpRequest &Request()
    {
        return _request;
    }

    // 接收Http请求
    void RecvHttpRequest(Buffer &buf)
    {
        // 一次性处理完Http请求，所以不break
        switch (_recv_status)
        {
        case RECV_HTTP_LINE:
            RecvHttpLine(buf);
        case RECV_HTTP_HEAD:
            RecvHttpHead(buf);
        case RECV_HTTP_BODY:
            RecvHttpBody(buf);
        case RECV_HTTP_OVER:
            break;
        }
    }

private:
    int _resp_status;            // 响应状态码
    HttpRecvStatus _recv_status; // 当前接收及解析所处的状态
    HttpRequest _request;        // Http请求
};

class HttpServer
{
    using Handler = std::function<void(const HttpRequest &, HttpResponse *)>;
    using Handlers = std::vector<std::pair<std::regex, Handler>>; // 采用Restful形式的接口

    // 错误处理
    void ErrorHandler(const HttpRequest &req, HttpResponse *resp)
    {
        std::string body;
        std::string path = _basedir + "/404.html";
        bool ret = Util::ReadFile(path, &body);
        if (ret == false)
        {
            return;
        }
        resp->SetContent(body, "text/html");
    }

    // 制作Http响应
    void MakeResponse(const PtrConnection &conn, const HttpRequest &req, HttpResponse *resp)
    {
        // 1.制作请求行(HTTP/1.x 200 OK\r\n)
        std::stringstream resp_str;
        resp_str << req._version << " " << std::to_string(resp->_status) << " "
                 << Util::StatuDesc(resp->_status) << "\r\n";

        // 2.完善请求头(防止传过来的响应没设置)
        if (req.IsShortConnection()) // 短连接
        {
            resp->SetHeader("Connection", "close");
        }
        else // 长连接
        {
            resp->SetHeader("Connection", "keep-alive");
        }
        if ((!resp->_body.empty()) && (!resp->HasHeader("Content-Length"))) // 正文长度
        {
            resp->SetHeader("Content-Length", std::to_string(resp->_body.size()));
        }
        if ((!resp->_body.empty()) && (!resp->HasHeader("Content-Type"))) // 正文类型
        {
            resp->SetHeader("Content-Type", "application/octet-stream"); // 默认二进制流
        }
        if (resp->_redirect_flag == true) // 设置了重定向
        {
            resp->SetHeader("Location", resp->_redirect_url);
        }

        // 3.制作请求头
        for (auto &head : resp->_headers)
        {
            resp_str << head.first << ": " << head.second << "\r\n"; // 单行请求头
        }
        resp_str << "\r\n"; // 空行

        // 4.请求体得外部自己提前设置
        resp_str << resp->_body;

        // 5.发送响应
        std::string data = resp_str.str();
        conn->Send(data.c_str(), data.size());
    }

    // 当前静态资源请求是否合法(外部调用)
    bool IsFileHandler(const HttpRequest &req)
    {
        if (_basedir.empty())
        {
            return false;
        }
        if (req._method != "GET" && req._method != "HEAD") // 请求方法是否合法
        {
            return false;
        }
        if (!Util::ValidPath(req._path)) // 请求路径是否合法
        {
            return false;
        }
        std::string req_path = _basedir + req._path; // 完整路径(避免修改原变量)
        if (req_path.back() == '/')                  // 请求根目录返回首页
        {
            req_path += "index.html";
        }
        if (!Util::IsRegular(req_path)) // 非普通文件
        {
            return false;
        }
        return true;
    }

    // 静态资源的请求处理
    void FileHandler(const HttpRequest &req, HttpResponse *resp)
    {
        std::string req_path = _basedir + req._path;
        if (req_path.back() == '/') // 请求根目录返回首页
        {
            req_path += "index.html";
        }
        bool ret = Util::ReadFile(req_path, &resp->_body);
        if (ret == false)
        {
            return;
        }
        std::string mime = Util::ExtMime(req_path);
        resp->SetHeader("Content-Type", mime);
    }

    // 事件派发器
    void Dispatcher(HttpRequest &req, HttpResponse *resp, Handlers &handlers)
    {
        for (auto &handler : handlers)
        {
            const std::regex &re = handler.first;                     // 正则方法
            const Handler &functor = handler.second;                  // 处理函数
            bool ret = std::regex_match(req._path, req._matches, re); // 根据注册的路由匹配
            if (ret == false)
            {
                continue;
            }
            functor(req, resp); // 到这里一定有调用的方法
            return;
        }
        resp->_status = 404; // 找不到该资源请求方法
    }

    // 根据请求方法的不同，分发不同的Restful接口
    void Route(HttpRequest &req, HttpResponse *resp)
    {
        // 静态资源请求
        if (IsFileHandler(req))
        {
            FileHandler(req, resp);
            return;
        }

        // 功能性请求
        std::string method = req._method;
        if (method == "GET" || method == "HEAD")
        {
            Dispatcher(req, resp, _get_route);
        }
        else if (method == "POST")
        {
            Dispatcher(req, resp, _post_route);
        }
        else if (method == "PUT")
        {
            Dispatcher(req, resp, _put_route);
        }
        else if (method == "DELETE")
        {
            Dispatcher(req, resp, _delete_route);
        }
        else
        {
            resp->_status = 405; //  方法不允许
        }
    }

    // 设置上下文
    void OnConnected(const PtrConnection &conn)
    {
        conn->SetContext(HttpContext());
        LOG(LOGLEVEL::DEBUG, "NEW CONNECTION %p", conn.get());
    }

    // 缓冲区数据解析+处理
    void OnMessage(const PtrConnection &conn, Buffer *buffer)
    {
        while (buffer->ReadAbleSize()) // 有数据就一直循环处理
        {
            // 获取上下文
            HttpContext *context = conn->GetContext()->get<HttpContext>();
            context->RecvHttpRequest(*buffer);
            HttpRequest &req = context->Request();
            HttpResponse resp = HttpResponse(context->RespStatus());
            if (resp._status >= 400) // 异常情况
            {
                ErrorHandler(req, &resp);
                MakeResponse(conn, req, &resp); // 制作响应发送数据
                context->Reset();
                buffer->MoveReadOffset(buffer->ReadAbleSize());
                conn->Shutdown();
                return;
            }
            if (context->RecvStatus() != RECV_HTTP_OVER)
            {
                // 当前请求还没有接收完整,则退出，等新数据到来再重新继续处理
                return;
            }
            Route(req, &resp); // 请求路由 + 业务处理
            if (resp._status >= 400) // 路由产生的404/405等错误同样走错误处理
            {
                ErrorHandler(req, &resp);
            }
            MakeResponse(conn, req, &resp);
            context->Reset(); // 重置上下文
            if (resp.IsShortConnection())
            {
                conn->Shutdown(); // 短连接就直接关闭
            }
        }
    }

public:
    HttpServer(int port, int timeout = DEFALT_TIMEOUT)
        : _server(port)
    {
        _server.EnableInactiveRelease(timeout);
        _server.SetConnectedCallBack(std::bind(&HttpServer::OnConnected, this, std::placeholders::_1));
        _server.SetMessageCallBack(std::bind(&HttpServer::OnMessage, this, std::placeholders::_1, std::placeholders::_2));
    }

    // 设置静态资源根目录
    void SetBaseDir(const std::string &path = default_basedir)
    {
        if (!Util::IsDirectory(path))
        {
            LOG(LOGLEVEL::ERR, "BaseDir Error!!!");
            exit(-1);
        };
        _basedir = path;
    }

    void RegisterGet(const std::string &pattern, const Handler &handler)
    {
        _get_route.push_back(std::make_pair(std::regex(pattern), handler));
    }

    void RegisterPost(const std::string &pattern, const Handler &handler)
    {
        _post_route.push_back(std::make_pair(std::regex(pattern), handler));
    }

    void RegisterPut(const std::string &pattern, const Handler &handler)
    {
        _put_route.push_back(std::make_pair(std::regex(pattern), handler));
    }

    void RegisterDelete(const std::string &pattern, const Handler &handler)
    {
        _delete_route.push_back(std::make_pair(std::regex(pattern), handler));
    }

    void SetThreadCount(int count)
    {
        _server.SetThreadCount(count);
    }

    void Listen()
    {
        _server.Start();
    }

private:
    Handlers _get_route;    // 处理GET请求
    Handlers _post_route;   // 处理POST请求
    Handlers _put_route;    // 处理PUT请求
    Handlers _delete_route; // 处理DELETE请求
    std::string _basedir;   // 静态资源根目录
    TcpServer _server;      //  TCP服务器
};