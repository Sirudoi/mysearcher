#include "../include/search.hpp"
// #include "../include/httplib.h"
#include "../include/log.hpp"
#include "../muduo/net/net_include/HttpServer.h"
#include "../muduo/net/net_include/HttpRequest.h"
#include "../muduo/net/net_include/HttpResponse.h"


using namespace ns_log;

const std::string output_path = "/home/wr/boost_search/data/raw_doc/raw.txt";
const std::string root_path = "../wwwroot/index.html";

// debug for muduo
#if 1
// 打印全部
bool benchmark = false;
void readHtml(std::string& body) {
    std::ifstream ifs(root_path.c_str());

    if (!ifs.is_open()) {
        LOG(WARN) << "open file failed" << std::endl;
        return;
    }

    body = std::string(std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>());
}


void onRequest(const HttpRequest& req, HttpResponse* resp) {
    std::cout << "Method:" << req.methodString() << " path:" << req.path() << std::endl;

    if (benchmark) {
        const std::map<std::string, std::string>& headers = req.headers();
        for (const auto& header : headers) {
            std::cout << header.first << ": " << header.second << std::endl;
        }
    }

    if (req.path() == "/") {
        resp->setStatusCode(HttpResponse::k200Ok);
        resp->setStatusMessage("OK");
        std::string body;
        readHtml(body);
        resp->setBody(body);
        resp->setContentType("text/html");
    }
    else if (req.path() == "/search") {
        std::cout << "query:" << req.query() << std::endl;

        resp->setStatusCode(HttpResponse::k200Ok);
        resp->setStatusMessage("OK");
        std::string body;
        readHtml(body);
        resp->setBody(body);
        resp->setContentType("text/html");
    }
}

#endif


int main()
{
    ns_search::Search* search = new ns_search::Search();
    if (!search->InitSearch(output_path))
        return 0;

    // 使用httplib库
#if 0 
    httplib::Server svr;
    // 设置首页
    svr.set_base_dir(root_path.c_str());
    svr.Get("/search", [&search](const httplib::Request& req, httplib::Response& resp){
        if (!req.has_param("word"))
        {
            resp.set_content("请输入搜索关键字", "text/plain;charser=utf-8");
            return;
        }
        // 获得用户提交参数
        std::string word = req.get_param_value("word");
        // 处理好后的序列化json字符串
        std::string json_str;
        search->SearchForKeyWord(word, &json_str);
        resp.set_content(json_str, "application/json;charset=utf-8");
    });

    svr.listen("0.0.0.0", 8081);
#endif

    // 使用muduo库
#if 1
    int numThreads = 3;
    EventLoop loop;
    
    // ----------------------------------------------------
    // 服务器需要使用INADDR_ANY作为ip绑定, 因此这里手动设置
    sockaddr_in local;
    local.sin_family = AF_INET;
    local.sin_port = ::htons(8080);
    local.sin_addr.s_addr = INADDR_ANY;
    // ----------------------------------------------------

    HttpServer server(&loop, InetAddress(local), "dummy");
    server.setHttpCallback([&search](const HttpRequest& req, HttpResponse* resp){
        std::cout << "Method:" << req.methodString() << " path:" << req.path() << std::endl;

        if (benchmark) {
            const std::map<std::string, std::string>& headers = req.headers();
            for (const auto& header : headers) {
                std::cout << header.first << ": " << header.second << std::endl;
            }
        }

        if (req.path() == "/") {
            resp->setStatusCode(HttpResponse::k200Ok);
            resp->setStatusMessage("OK");
            std::string body;
            readHtml(body);
            resp->setBody(body);
            resp->setContentType("text/html");
        }
        else if (req.path() == "/search") {
            // 从?word=位置开始截取输入参数
            std::string word = req.query().substr(6);
            LOG(INFO) << "Get current word:" << word << std::endl;
            std::string json_str;
            search->SearchForKeyWord(word, &json_str);
            resp->setBody(json_str);
            resp->setContentType("application/json;charset=utf-8");
        }
    });

    // server.setHttpCallback(onRequest);
    server.setThreadNum(numThreads);
    server.start();
    loop.loop();
#endif

    return 0;
}