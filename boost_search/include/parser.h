#pragma once
#include <string>
#include <vector>
#define SEP "\3"

const std::string original_path = "/home/wr/boost_search/data/original_doc";
const std::string output_path = "/home/wr/boost_search/data/raw_doc/raw.txt";

typedef struct DocInfo {
    std::string title;  //标题
    std::string content;//正文
    std::string url;    //文档url
} DocInfo_t;

bool EnumFilesName(const std::string& src_path, std::vector<std::string>* files_list); //枚举带路径的文件名
bool ParseDoc(const std::vector<std::string>& files_list, std::vector<DocInfo_t>* results_list); //解析.html文档内容，解析title content url
bool IntegrateDoc(const std::string& output_path, const std::vector<DocInfo_t>& results_list); //整合所有文档内容到一个文件中

bool ParseTitle(const std::string& result, std::string& title); //解析.html文档内容->解析title
bool ParseContent(const std::string& result, std::string& content); //解析.html文档内容->解析content
bool ParseUrl(const std::string& result, std::string& url); //解析.html文档内容->解析url

void ShowDoc(const DocInfo_t& doc);