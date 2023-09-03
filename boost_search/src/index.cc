#include "../include/index.h"
#include "../include/threadpool.h"
#include <future>

bool IndexFinish() {
    return static_cast<bool>(ThreadPool::getInstance()->empty());
}

namespace ns_index {

int count = 0;  // for debug
int sum = 0;    // for debug
Index* Index::ins_ = nullptr;
std::mutex Index::mtx_;

Index::Index() {}

Index::~Index() {}

/**
 * @brief 获取正排索引
 * 
 * @param doc_id 文档id
 * @return DocInfo* 
 */
DocInfo* Index::GetForwardList(const uint64_t& doc_id) {
    if (doc_id >= forward_list_.size()) {
        LOG(ERROR) << "Doc id 越界" << std::endl;

        return nullptr;
    }

    return &forward_list_[doc_id];
}

/**
 * @brief 获取倒排索引
 * 
 * @param word 关键字
 * @return InvertedList* 
 */
Index::InvertedList* Index::GetInverterList(const std::string& word) {
    auto iter = inverted_list_.find(word);
    if (iter == inverted_list_.end()) {
        LOG(WARN) << "Inverter List Get Fail by:" << word << std::endl;

        return nullptr;
    }

    // 返回拉链
    return &(iter->second);
}

/**
 * @brief           构建正排索引和倒排索引
 * 
 * @param out_path  清晰完毕的文档路径
 * @return true     构建成功
 * @return false    构建失败
 */
bool Index::BuildIndex(const std::string& out_path) {
    std::ifstream ifs(out_path, std::ifstream::in | std::ifstream::binary);
    if (!ifs.is_open()) {
        LOG(WARN) << "raw.txt open fail" << std::endl;
        return false;
    }

    std::string line;
    while (std::getline(ifs, line)) {
        ThreadPool::getInstance()->join_queue([=](){ BuildOneDocIndex(line); });
    }
    LOG(INFO) << "ThreadPool size = " << ThreadPool::getInstance()->size() << std::endl;
    // LOG(INFO) << " 索引建立完毕" << std::endl;
    // TODO:检测是否建立完毕

    // for (;;) {
    //     {
    //         std::unique_lock<std::mutex> lock(mtx_);
    //         cond_.wait(lock, [](){ return ThreadPool::getInstance()->empty(); });

    //         if (ThreadPool::getInstance()->empty())
    //             break;
    //     }
    //     LOG(INFO) << "ThreadPool size = " << ThreadPool::getInstance()->size() << std::endl;
    // }

    return true;
}

/**
 * @brief       构建单个文档的正排索引和倒排索引
 * @param doc   文档内容
 */
bool Index::BuildOneDocIndex(std::string per_doc) {
    DocInfo* doc = BulidForwardIndex(per_doc);
    if (doc == nullptr) {
        std::cout << "BulidForwardIndex fail" << std::endl;
        return false;
    }

    BuildInverterIndex(*doc);

    return true;
}


/**
 * @brief           获取单例
 * 
 * @param out_path  清洗文档所在路径
 * @return Index*   单例对象
 */
Index* Index::GetInstance(const std::string& out_path) {
    // 双重检测，防止出现下面情况
    // A线程未创建单例，创建单例前被切走，此时B进程判断为空进入创建单例，接着A进程继续执行单例创建任务
    if (ins_ == nullptr) {
        mtx_.lock();
        if (ins_ == nullptr) {
            ins_ = new ns_index::Index();
            ins_->BuildIndex(out_path);
        }
        mtx_.unlock();
    }

    return ins_;
}

/**
 * @brief           构建正排索引
 * 
 * @param line      单个文档
 * @return DocInfo* 
 */
DocInfo* Index::BulidForwardIndex(const std::string& line) {
    // 1.切分字符串，以\3切分标题内容和网页链接
    LOG(INFO) << line.substr(0, 50)<<'\n';
    std::vector<std::string> result;
    ns_util::StringUtil::CutString(line, &result, "\3");
    if (result.size() != 3) {
        return nullptr;
    }

    // 2.用切分内容构建正排文档
    DocInfo doc;
    doc.title = result[0];
    doc.content = result[1];
    doc.url = result[2];
    doc.doc_id = forward_list_.size();

    // 3.插入到正排索引中
    {
        // STL库都是线程不安全的
        std::unique_lock<std::mutex> lock(mtx_);
        forward_list_.emplace_back(std::move(doc));
    }

    return &forward_list_.back();  // 返回最顶上节点
}

/**
 * @brief           构建单个倒排索引
 * 
 * @param doc       单个文档
 * @return true
 * @return false 
 */
bool Index::BuildInverterIndex(const DocInfo& doc) {
    // LOG(INFO) << doc.content.size() << " " << doc.title.size() << " " << doc.url.size() << std::endl;

    // 暂存分词和其词频的映射
    std::unordered_map<std::string, WordCnt> cnt_map;

    // 1.对title分词
    std::vector<std::string> title_key_word;
    ns_util::JiebaUtil::CutKeyWord(doc.title, &title_key_word);

    // 2.计算title每个分词的词频
    for (auto& s : title_key_word) {
        boost::to_lower(s);  // 统一将关键词转化为小写
        cnt_map[s].title_cnt++;
    }

    // 3.对content分词
    std::vector<std::string> content_key_word;
    ns_util::JiebaUtil::CutKeyWord(doc.content, &content_key_word);

    // 4.计算content的每个分词的词频
    for (auto& s : content_key_word) {
        boost::to_lower(s);
        cnt_map[s].content_cnt++;
    }

    // 5.构建这个文档每个分词的倒排拉链
    {
        // STL线程不安全
        std::unique_lock<std::mutex> lock(mtx_);
        for (auto& iter : cnt_map) {
        // 新建一个倒排元素
        // word是这个关键词、doc_id对应当前文档id、weight是该关键词与doc_id文档下的相关性
        InvertedElem ivr;
        ivr.word = iter.first;
        ivr.doc_id = doc.doc_id;
        ivr.weight = ns_util::RelativityUtil::CalWeight(
            iter.second.title_cnt, iter.second.content_cnt);  // 计算相关性

        inverted_list_[iter.first].push_back(
            std::move(ivr));  // 插入到这个关键词倒排拉链的vector中
        }
        // For Debug
        ++count;
        if (count == 100) {
            sum += count;
            count %= 100;
            LOG(INFO) << " 索引已建立:" << sum << std::endl;
        }
    }


    return true;
}

}