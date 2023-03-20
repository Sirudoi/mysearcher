## 基于BOOST的站内搜索引擎

### 项目展示

页面展示

![image-20230320142453891](D:\Typora\images\image-20230320142453891.png)	搜索结果展示

![](D:\Typora\images\image-20230320142622948.png)	

### 宏观原理

![image-20230320142912771](D:\Typora\images\image-20230320142912771.png)	

- 下载好`boost`的文档内容，将所有`html`里面的有效内容提取出来之后

- 建立正排索引和倒排索引

- 获取用户关键词，根据用户关键词拉取倒排索引

- 根据倒排索引拉取正排索引

- 根据正排索引拿到文档内容，然后提取文档标题和摘要，构建跳转URL
- 将搜索内容按照权值排序后，构建网页，返回给用户

### 正排索引和倒排索引介绍

> 例如下面有两个文档：
>
> - 文档1：快速排序是一个优秀的排序算法
> - 文档2：陈奕迅是一个优秀的歌唱家

| 文档ID | 文档内容                     |
| ------ | ---------------------------- |
| 1      | 快速排序是一个优秀的排序算法 |
| 2      | 陈奕迅是一个优秀的歌唱家     |

> 将这两个文档分词：
>
> - 快速排序是一个优秀的排序算法、
>
> 快速，排序，快速排序，优秀，算法，排序算法...
>
> - 陈奕迅是一个优秀的歌唱家
>
> 陈奕迅，优秀，歌唱家，优秀的歌唱家...

根据分词结果，可以建立倒排索引

| 关键词       | 文档         |
| ------------ | ------------ |
| 快速         | 文档1        |
| 排序         | 文档1        |
| 快速排序     | 文档1        |
| 优秀         | 文档1，文档2 |
| 算法         | 文档1        |
| 排序算法     | 文档1        |
| 陈奕迅       | 文档2        |
| 歌唱家       | 文档2        |
| 优秀的歌唱家 | 文档2        |

假设此时用户输入：优秀。

- 优秀根据倒排索引拉取
- 拉取出文档1，文档2
- 根据文档1，获取文档内容：`快速排序是一个优秀的排序算法`
- 根据文档2，获取文档内容：`陈奕迅是一个优秀的歌唱家`
- 将文档内容回显给用户

以上就是本迷你搜索引擎的技术原理。

### 去标签模块

去boost官网下载好文档内容，将所有`html`文件保存到`/boost_search/data/original_doc`中

> 去标签模块主要工作为：
>
> - 列举每个带路径的`html`文件名
> - 将`html`内容解析出来，按照标题，内容，源文`url`格式存储
> - 将所有解析出来的内容，整合到一个文档中

#### 列举带路径文件名

将每个html文件附上路径，存储起来，便于后面处理

```c++
//parser.cc
bool EnumFilesName(const std::string &src_path, std::vector<std::string> *files_list)
{
    boost::filesystem::path root_path(src_path);
    // 检查路径是否存在，根路径不存在直接返回
    if (!boost::filesystem::exists(root_path))
    {
        std::cerr << "original path is not exit!" << std::endl;
        return false;
    }

    // 定义空迭代器，用于迭代器递归结束
    boost::filesystem::recursive_directory_iterator end;
    // 递归每一个文件内容
    for (boost::filesystem::recursive_directory_iterator it(root_path); it != end; ++it)
    {
        // 不是常规文件，直接跳过
        if (!boost::filesystem::is_regular_file(*it))
            continue;

        // 筛选，带路径文件名结束后缀是:.html，不是.html后缀的文件直接跳过
        if (it->path().extension() != ".html")
            continue;
        // 将.html后缀文件的带路径路径名以字符串插入到file_list中
        files_list->emplace_back(it->path().string());
        // ===============================================
        // Debug:
        // std::cout << it->path().string() << std::endl;
    }

    return true;
}
```

#### 将`html`内容解析出来

根据上一步处理好的带路径文件名，把所有文件进行去标签处理。处理完的内容以`DocInfo_t`结构体形势存放，并且把所有`DocInfo_t`存放起来

```c++
typedef struct DocInfo {
    std::string title;  //标题
    std::string content;//正文
    std::string url;    //文档url
} DocInfo_t;

bool ParseDoc(const std::vector<std::string> &files_list, std::vector<DocInfo_t> *results_list)
{
    for (auto &file_path : files_list)
    {
        // 1.读文件，把文件内容读取到内存
        std::string result;
        if (!ns_util::FileUtil::ReadFile(file_path, result))
        {
            std::cout << "Read Error:" << file_path << std::endl;
            continue;
        }

        DocInfo_t doc;
        // 2.提取title
        if (!ParseTitle(result, doc.title))
        {
            std::cout << "Title Error:" << file_path << std::endl;
        }

        // 3.提取content
        if (!ParseContent(result, doc.content))
        {
            std::cout << "Content Error:" << file_path << std::endl;
        }

        // 4.解析url
        if (!ParseUrl(file_path, doc.url))
        {
            std::cout << "Url Error:" << file_path << std::endl;
        }

        // Debug
        // std::cout << file_path << std::endl;
        // ShowDoc(doc);
        // break;
        results_list->push_back(std::move(doc));
    }

    return true;
}

// 解析文档的title
bool ParseTitle(const std::string &result, std::string &title)
{
    // 寻找<title>的位置
    int begin = result.find("<title>");
    if (begin == std::string::npos)
    {
        return false;
    }

    // 寻找</title>的位置
    int end = result.find("</title>");
    if (end == std::string::npos)
    {
        return false;
    }
    // begin += std::string("<title>").size();
    begin += 7;

    title = result.substr(begin, end - begin); // 提取<title> ~ </title>中间的内容

    return true;
}

// 解析文档content
bool ParseContent(const std::string &result, std::string &content)
{
    //状态基
    enum status
    {
        LABLE, //标签
        CONTENT //正文
    };

    enum status s = LABLE;
    for (char c : result)
    {
        switch (s)
        {
        case LABLE:
            if (c == '>')
                s = CONTENT; //遇到>，接下来的内容是正文
            break;
        case CONTENT:
            if (c == '<')
                s = LABLE; //遇到<，接下来的内容是标签
            else
            {
                //否则是正文内容，写入到content中，如果是\n，改成空格
                if (c == '\n')
                    c = ' ';
                content += c;
            }
            break;
        default:
            break;
        }
    }

    return true;
}

//解析url
bool ParseUrl(const std::string& file_path, std::string& url)
{
    //构建url头
    std::string url_head = "https://www.boost.org/doc/libs/1_81_0/doc/html";
    //url尾巴为传入的file_path，减去"/home/wr/boost_search/data/original_doc"的内容
    std::string url_tail = file_path.substr(original_path.size());
    url = url_head + url_tail;

    return true;   
}
```

#### 整合去标签后的结果

- 将上面去标签后的结果整合到一个文档中

- 直接整合会出现粘包问题，我这边指定`'\3'`作为`标题，描述，还有url`的分隔符，以`\n`为每个文档的分隔符

```c++
bool IntegrateDoc(const std::string &output_path, const std::vector<DocInfo_t> &results_list)
{
    std::ofstream ofs(output_path, std::ofstream::out | std::ofstream::binary);
    if (!ofs.is_open())
    {
        std::cout << output_path << ":open fail!" << std::endl;
        return false;
    }

    for (auto& doc : results_list)
    {
        //title\3content\3url\n
        //以上面这种形势来写入文件，便于后续读取
        std::string out_str;
        out_str += doc.title;
        out_str += SEP;
        out_str += doc.content;
        out_str += SEP; 
        out_str += doc.url;
        out_str += '\n';

        ofs.write(out_str.c_str(), out_str.size());
    }
    ofs.close();

    return true;
}
```

### 建立索引模块

#### 构建正排

- 上面整合结果是以\n来作为每个文档的分割符
- 外部按行读取就是每个文档的结果
- 每个文档中标题，文档内容，跳转`url`以`\3`为分隔符
- 用`\3`切分，构建正排索引

```c++
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
```

#### 构建倒排

- 提取文档内容，利用`jieba`分词进行分词
- 计算分词后，每个词的词频，结果用`WordCnt`存储
- 词频统计需要分别计算出现在标题和出现在正文的次数，因为出现在标题和出现在正文对该词的权重有关系
- 统计的时候需要统一转成小写，因为可能输入的关键词含大小，大小写混合时不会匹配
- 构建倒排索引时，按照词频统计这个词在对应文档的权重，这个是后续结果排序的基础

```c++
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
```

#### 返回正排&&倒排索引结果

- 根据关键词，拉取倒排拉链，获得倒排结果
- 根据倒排结果获得文档id，获得文档内容

```c++
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
```

### 搜索引擎模块

- 上述索引模块设计为单例，在搜索模块利用单例指针，调用`返回正排&&倒排索引结果`后即可获得搜索结果
- 在此之前，需要对输入的关键字也进行分词
- 分词之后，可能多个词对应同一个文档，这样子这个文档会出现多次，因此获得全部正排后需要去重
- 如果多个分词都对应这个文档，那么也需要提高这个文档的权重，让其尽量往前面排

#### 将输入关键字分词，拉取倒排和正排

```c++
typedef struct InvertedElemUnique
{
    int weight;
    uint64_t doc_id;
    std::vector<std::string> word_list;

} UniqueElem;

void SearchForKeyWord(const std::string& word, std::string* json_str)
{
    //1.对关键词分词
    std::vector<std::string> cut_result;
    ns_util::JiebaUtil::CutKeyWord(word, &cut_result);

    //2.根据搜索词的分词，拉取倒排拉链
    std::vector<ns_index::UniqueElem> all_inverter_list;
    std::unordered_map<uint64_t, ns_index::UniqueElem> unique_list; // 去重累加计算

    for (auto& s : cut_result)
    {
        //构建index的时候是全部小写化的，所以这里需要对搜索词先转化成小写
        boost::to_lower(s);

        //根据关键词获取倒排拉链
        ns_index::Index::InvertedList* list = ins->GetInverterList(s);
        if (list == nullptr)
            continue;

        // 可能有多个词对应一个文档，此时就会出现 {inelem1，inelem2，inelem1}因此后续查找会找出相同的html
        // all_inverter_list.insert(all_inverter_list.end(), list->begin(), list->end()); //可能会重复

        // 对倒排拉链结果去重
        for (const auto& e : *list)
        {
            // 插入文档id
            auto& value = unique_list[e.doc_id];
            value.doc_id = e.doc_id;
            value.weight += e.weight; // 如果不存在，正常复制；存在则累加权重
            value.word_list.push_back(e.word); // 不重复则只插入一个，重复则会插入重复的搜索分词
        }
    }
    // 生成去重后的倒排
    for (auto& e : unique_list)
    {
        all_inverter_list.emplace_back(std::move(e.second));
    }

    //3.汇总所有倒排拉链里的元素，按照weight排序
    std::sort(all_inverter_list.begin(), all_inverter_list.end(), [](ns_index::UniqueElem& e1, ns_index::UniqueElem&e2){
        return e1.weight > e2.weight;
    });

    // 没去重
    // std::sort(all_inverter_list.begin(), all_inverter_list.end(), [](const ns_index::InvertedElem& e1, const ns_index::InvertedElem& e2){
    //     return e1.weight > e2.weight;
    // });

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
```

#### 获取摘要

- 获得正排后，还需要显示摘要
- 这里较为简单的，直接获取搜索关键字前50和后100个字符的内容

```c++
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
```

### 服务模块

- 根据输入关键字，调用相关接口

```c++
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
```

