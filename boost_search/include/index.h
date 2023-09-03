#ifndef INCLUDE_INDEX_H
#define INCLUDE_INDEX_H
#include <fstream>
#include <iostream>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>
#include <condition_variable>


#include "./log.hpp"
#include "./util.h"

namespace ns_index {
    using namespace ns_log;

    // 去重倒排元素，可能一个词分词后，找到很多个id相同的文档
    // 将id相同文档去重，并且把分词放在一个数组中
    // 搜索权值累加
    typedef struct InvertedElemUnique {
        int weight;
        uint64_t doc_id;
        std::vector<std::string> word_list;

    } UniqueElem;

    // 正排元素
    struct DocInfo {
        std::string title;
        std::string content;
        std::string url;
        uint64_t doc_id;
    };

    // 倒排元素
    struct InvertedElem {
        std::string word;
        uint64_t doc_id;
        int weight;
    };

    // 词频结构体
    struct WordCnt {
        size_t title_cnt;
        size_t content_cnt;
        WordCnt() : title_cnt(0), content_cnt(0) {}
    };

    class Index {
    public:
        typedef std::vector<InvertedElem> InvertedList;

    private:
        Index();
        ~Index();
        Index(const Index& index) = delete;
        Index& operator=(const Index& index) = delete;

    public:
        DocInfo* GetForwardList(const uint64_t& doc_id);                // 获取正排索引
        InvertedList* GetInverterList(const std::string& word);         // 获取倒排索引
        bool BuildIndex(const std::string& out_path);                   // 构建索引
        bool BuildOneDocIndex(std::string per_doc);                         // 构建单个索引
        static Index* GetInstance(const std::string& out_path);         // 获取单例

    private:
        DocInfo* BulidForwardIndex(const std::string& line);            // 构建单个正排索引
        bool BuildInverterIndex(const DocInfo& doc);                    // 构建单个倒排索引

    private:
        std::vector<DocInfo> forward_list_;                             // 正排索引, 数组下标代表文档ID
        std::unordered_map<std::string, InvertedList> inverted_list_;   // 倒排索引, 关键字和一组倒排元素的映射

        static Index* ins_;
        static std::mutex mtx_;
        std::condition_variable cond_;
    };


}  // namespace ns_index
#endif // INCLUDE_INDEX_H