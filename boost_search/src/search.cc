#include "../include/search.hpp"
#include "../include/httplib.h"
#include "../include/log.hpp"

using namespace ns_log;

const std::string output_path = "/home/wr/boost_search/data/raw_doc/raw.txt";
const std::string root_path = "./wwwroot";

int main()
{
    ns_search::Search* search = new ns_search::Search();
    if (!search->InitSearch(output_path))
        return 0;
    
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

    return 0;
}