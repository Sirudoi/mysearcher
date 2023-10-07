#include "../include/util.h"

namespace ns_util {

/**
 * @brief               从file_path读取每一行到out形成一行
 * @param file_path     读路径
 * @param out           输出路径
 */
bool FileUtil::ReadFile(const std::string& file_path, std::string& out) 
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

/**
 * @brief           切分标题 正文 url
 * @param src       源文档
 * @param result    结果
 * @param sep       TODO
 */
void StringUtil::CutString(const std::string& src, 
                        std::vector<std::string>* result, 
                        const std::string& sep) {
    boost::split(*result, src, boost::is_any_of(sep), boost::token_compress_on);
}

/**
 * @brief           jieba分词
 * @param str       需要切分的字符串
 * @param result    切分结果
 */
void JiebaUtil::CutKeyWord(const std::string& str, 
                        std::vector<std::string>* result) {
    jieba.CutForSearch(str, *result);
}

// 静态成员类外初始化
cppjieba::Jieba JiebaUtil::jieba(DICT_PATH, 
                                HMM_PATH, 
                                USER_DICT_PATH, 
                                IDF_PATH, 
                                STOP_WORD_PATH);

/**
 * @brief               计算相关性
 * @param title_word    某个词在标题出现的次数
 * @param content_word  某个词在正文出现的次数
 */
int RelativityUtil::CalWeight(const size_t& title_word, const size_t& content_word) {
    return 10 * title_word + content_word;
}

}