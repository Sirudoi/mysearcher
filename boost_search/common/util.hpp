#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <boost/algorithm/string.hpp> //切分字符串
#include "../cppjieba/Jieba.hpp"

namespace ns_util
{
    //jieba词库的路径
    const char* const DICT_PATH = "dict/jieba.dict.utf8";
    const char* const HMM_PATH = "dict/hmm_model.utf8";
    const char* const USER_DICT_PATH = "dict/user.dict.utf8";
    const char* const IDF_PATH = "dict/idf.utf8";
    const char* const STOP_WORD_PATH = "dict/stop_words.utf8";

    //读文件
    class FileUtil 
    {
    public:
        static bool ReadFile(const std::string& file_path, std::string& out) 
        {
            std::ifstream ifs(file_path, std::ios::in);
            if (!ifs.is_open()) {
                std::cerr << "open file error#" << file_path << std::endl;
                return false;
            }

            std::string line;
            while (std::getline(ifs, line)) {
                out += line;
            }
            ifs.close();

            return true;
        }
    };

    //切分title content url
    class StringUtil
    {
    public:
        static void CutString(const std::string& src, std::vector<std::string>* result, const std::string& sep)
        {
            boost::split(*result, src, boost::is_any_of(sep), boost::token_compress_on);
        }
    };

    //jieba分词
    class JiebaUtil
    {
    public:
        static void CutKeyWord(const std::string& str, std::vector<std::string>* result)
        {
            jieba.CutForSearch(str, *result);
        }
    private:
        static cppjieba::Jieba jieba; //定义为静态，免得每次都要初始化
    };
    //静态成员类外初始化
    cppjieba::Jieba JiebaUtil::jieba(DICT_PATH, HMM_PATH, USER_DICT_PATH, IDF_PATH, STOP_WORD_PATH);

    //计算相关性
    class RelativityUtil
    {
    public:
        static int CalWeight(const size_t& title_word, const size_t& content_word)
        {
            return 10 * title_word + content_word;
        }
    };
}