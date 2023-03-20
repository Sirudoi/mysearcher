#pragma once
#include <mutex>
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <unordered_map>

#include "./common/util.hpp"
#include "./common/log.hpp"

namespace ns_index
{
    class Index;
    struct ThreadArg
    {
        Index *index;
        int start_line;
        int read_line;
        std::ifstream *ifs;
        int *count;
        int *sum;
    };
    using namespace ns_log;

    // 去重倒排元素，可能一个词分词后，找到很多个id相同的文档
    // 将id相同文档去重，并且把分词放在一个数组中
    // 搜索权值累加
    typedef struct InvertedElemUnique
    {
        int weight;
        uint64_t doc_id;
        std::vector<std::string> word_list;

    } UniqueElem;

    // 正排元素
    struct DocInfo
    {
        std::string title;
        std::string content;
        std::string url;
        uint64_t doc_id;
    };

    // 倒排元素
    struct InvertedElem
    {
        std::string word;
        uint64_t doc_id;
        int weight;
    };

    // 词频结构体
    struct WordCnt
    {
        size_t title_cnt;
        size_t content_cnt;
        WordCnt() : title_cnt(0), content_cnt(0) {}
    };

    class Index
    {
    public:
        typedef std::vector<InvertedElem> InvertedList;

    private:
        Index(){};
        ~Index(){};
        Index(const Index &index) = delete;
        Index &operator=(const Index &index) = delete;

    public:
        // 根据文档ID，返回文档内容
        DocInfo *GetForwardList(const uint64_t &doc_id)
        {
            if (doc_id >= forward_list.size())
            {
                LOG(ERROR) << "Doc id 越界" << std::endl;

                return nullptr;
            }

            return &forward_list[doc_id];
        }

        // 根据关键字word，获得倒排拉链
        InvertedList *GetInverterList(const std::string &word)
        {
            auto iter = inverted_list.find(word);
            if (iter == inverted_list.end())
            {
                LOG(WARN) << "Inverter List Get Fail by:" << word << std::endl;

                return nullptr;
            }

            // 返回拉链
            return &(iter->second);
        }

        // 根据output_path文件内容，构建正排和倒排索引
        bool BuildIndex(const std::string &out_path)
        {
            std::ifstream ifs(out_path, std::ifstream::in | std::ifstream::binary);
            if (!ifs.is_open())
            {
                LOG(WARN) << "raw.txt open fail" << std::endl;
                return false;
            }

            int count = 0; // for debug
            int sum = 0;   // foe debug

            std::string line;
            while (std::getline(ifs, line))
            {
                DocInfo* doc = BulidForwardIndex(line);
                if (doc == nullptr)
                {
                    std::cout << "BulidForwardIndex fail" << std::endl;
                    continue;
                }

                BuildInverterIndex(*doc);
                count++;
                if (count == 100)
                {
                    sum += count;
                    count %= 100;
                    LOG(INFO) << " 索引已建立:" << sum << std::endl;
                }
            }
            LOG(INFO) << " 索引建立完毕"<< std::endl;

            return true;
        }

    // 获取单例
    static Index *GetInstance(const std::string &out_path)
    {
        // 双重检测，防止出现下面情况
        // A线程未创建单例，创建单例前被切走，此时B进程判断为空进入创建单例，接着A进程继续执行单例创建任务
        if (ins == nullptr)
        {
            mtx.lock();
            if (ins == nullptr)
            {
                ins = new ns_index::Index();
                ins->BuildIndex(out_path);
            }
            mtx.unlock();
        }

        return ins;
    }

private:
    // 构建正排
    DocInfo *BulidForwardIndex(const std::string &line)
    {
        // 1.切分字符串，以\3切分标题内容和网页链接
        std::vector<std::string> result;
        ns_util::StringUtil::CutString(line, &result, "\3");
        if (result.size() != 3)
        {
            return nullptr;
        }

        // 2.用切分内容构建正排文档
        DocInfo doc;
        doc.title = result[0];
        doc.content = result[1];
        doc.url = result[2];
        doc.doc_id = forward_list.size();

        // 3.插入到正排索引中
        forward_list.push_back(std::move(doc));

        return &forward_list.back(); // 返回最顶上节点
    }

    // 构建倒排
    bool BuildInverterIndex(const DocInfo &doc)
    {
        std::unordered_map<std::string, WordCnt> cnt_map; // 暂存分词和其词频的映射

        // 1.对title分词
        std::vector<std::string> title_key_word;
        ns_util::JiebaUtil::CutKeyWord(doc.title, &title_key_word);

        // 2.计算title每个分词的词频
        for (auto &s : title_key_word)
        {
            boost::to_lower(s); // 统一将关键词转化为小写
            cnt_map[s].title_cnt++;
        }

        // 3.对content分词
        std::vector<std::string> content_key_word;
        ns_util::JiebaUtil::CutKeyWord(doc.content, &content_key_word);

        // 4.计算content的每个分词的词频
        for (auto &s : content_key_word)
        {
            boost::to_lower(s);
            cnt_map[s].content_cnt++;
        }

        // 5.构建这个文档每个分词的倒排拉链
        for (auto &iter : cnt_map)
        {
            // 新建一个倒排元素
            // word是这个关键词、doc_id对应当前文档id、weight是该关键词与doc_id文档下的相关性
            InvertedElem ivr;
            ivr.word = iter.first;
            ivr.doc_id = doc.doc_id;
            ivr.weight = ns_util::RelativityUtil::CalWeight(iter.second.title_cnt, iter.second.content_cnt); // 计算相关性

            inverted_list[iter.first].push_back(std::move(ivr)); // 插入到这个关键词倒排拉链的vector中
        }

        return true;
    }

private:
    // 正排索引，数组小标代表文档ID
    std::vector<DocInfo> forward_list;
    // 倒排索引，一个关键字和一组倒排元素的映射关系
    std::unordered_map<std::string, InvertedList> inverted_list;

    static Index *ins;
    static std::mutex mtx;
};
Index *ns_index::Index::ins = nullptr;
std::mutex ns_index::Index::mtx;


}