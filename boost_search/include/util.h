#ifndef INCLUDE_UTIL_H
#define INCLUDE_UTIL_H

#include <boost/algorithm/string.hpp>  //切分字符串
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include "../cppjieba/Jieba.hpp"

namespace ns_util {

// jieba词库的路径
const char* const DICT_PATH = "../dict/jieba.dict.utf8";
const char* const HMM_PATH = "../dict/hmm_model.utf8";
const char* const USER_DICT_PATH = "../dict/user.dict.utf8";
const char* const IDF_PATH = "../dict/idf.utf8";
const char* const STOP_WORD_PATH = "../dict/stop_words.utf8";

// 读文件
class FileUtil {
public:
    static bool ReadFile(const std::string& file_path, std::string& out);
};

// 切分title content url
class StringUtil {
public:
    static void CutString(const std::string& src,
                          std::vector<std::string>* result,
                          const std::string& sep);
};

// jieba分词
class JiebaUtil {
public:
    static void CutKeyWord(const std::string& str,
                           std::vector<std::string>* result);

private:
    static cppjieba::Jieba jieba;  // 定义为静态，免得每次都要初始化
};

// 计算相关性
class RelativityUtil {
public:
    static int CalWeight(const size_t& title_word, const size_t& content_word);
};

}  // namespace ns_util

#endif  // INCLUDE_UTIL_H