#ifndef SEARCH_INCLUDE
#define SEARCH_INCLUDE
#pragma once
#include <algorithm>
#include <jsoncpp/json/json.h>
#include "index.hpp"
#include "util.hpp"


namespace ns_search
{
    class Search
    {   
    public:
        Search() {}
        ~Search() {}

        bool InitSearch(const std::string& out_path)
        {
            ins = ns_index::Index::GetInstance(out_path);
            if (ins == nullptr)
            {
                return false;
            }

            return true;
        }

        void SearchForKeyWord(const std::string& word, std::string* json_str)
        {
            //1.对关键词分词
            std::vector<std::string> cut_result;
            ns_util::JiebaUtil::CutKeyWord(word, &cut_result);

            //2.根据搜索词的分词，拉取倒排拉链
            ns_index::Index::InvertedList all_inverter_list;
            for (auto& s : cut_result)
            {
                //构建index的时候是全部小写化的，所以这里需要对搜索词先转化成小写
                boost::to_lower(s);

                //根据关键词获取倒排拉链
                ns_index::Index::InvertedList* list = ins->GetInverterList(s);
                if (list == nullptr)
                    continue;

                //TODO:可能有多个词对应一个文档，此时就会出现 {inelem1，inelem2，inelem1}因此后续查找会找出相同的html
                all_inverter_list.insert(all_inverter_list.end(), list->begin(), list->end()); //将所有倒排节点重新整合
            }

            //3.汇总所有倒排拉链里的元素，按照weight排序
            std::sort(all_inverter_list.begin(), all_inverter_list.end(), [](const ns_index::InvertedElem& e1, const ns_index::InvertedElem& e2){
                return e1.weight > e2.weight;
            });

            //4.根据查找结果构建返回的json串
            Json::Value root;
            for (auto& e : all_inverter_list)
            {
                ns_index::DocInfo* doc = ins->GetForwardList(e.doc_id);
                if (doc == nullptr)
                    continue;
                
                Json::Value item;
                item["title"] = doc->title;
                //TODO:处理正文，获取摘要
                item["content"] = GetDesc(doc->content, word);
                item["url"] = doc->url;
                root.append(item);
            }
            Json::StyledWriter wtr;
            *json_str = wtr.write(root);
            
        }
    private:
        //截取关键词前50，后100的
        //TODO:改进获取摘要的算法
        std::string GetDesc(const std::string& content, const std::string& word)
        {
            std::size_t begin = 0; //默认截取的开头
            std::size_t end = content.size(); //默认截取的结尾

            // std::size_t pos = content.find(word); //这样不能自动屏蔽大小写
            // 用search接口，通过回调来控制忽略大小写
            auto iter = std::search(content.begin(), content.end(), word.begin(), word.end(), [](const char& s1, const char& s2) {
                return (std::tolower(s1) == std::tolower(s2));
            });

            std::size_t pos = std::distance(content.begin(), iter); //计算两个迭代器的距离
            if (pos > 50)
            {
                //pos前面有50字符，往前截50
                begin = pos - 50;
            }
            if (pos + 100 < end)
            {
                //pos后面有100字符，往后截100
                end = pos + 100;
            }
            // std::cout << " begin:" << begin << " end:" << end << std::endl; for debug

            return content.substr(begin, end - begin);

        }

    private:
        static ns_index::Index* ins;
    };
    ns_index::Index* ns_search::Search::ins = nullptr;
}

#endif