#include <iostream>
#include <fstream>
#include <boost/filesystem.hpp>
#include "../include/parser.h"
#include "../include/util.h"

int main()
{
    std::vector<std::string> files_list;
    // 列举每个带路径的文件名，便于后续对每个文件进行相关去标签操作
    if (!EnumFilesName(original_path, &files_list))
    {
        std::cerr << "Enum Filename error" << std::endl;
        return 1;
    }

    // 把每个文档文件内容解析出来，解析的格式为DocInfo，并且存放到result列表
    std::vector<DocInfo_t> results_list;
    if (!ParseDoc(files_list, &results_list))
    {
        std::cerr << "Parser Docment error" << std::endl;
        return 2;
    }

    // 将文件列表所有内容整合到一个文件中，文档之前用\3区分
    if (!IntegrateDoc(output_path, results_list))
    {
        std::cerr << "Integrate Docment error" << std::endl;
        return 3;
    }

    return 0;
}

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

// 解析文档的title
bool ParseTitle(const std::string &result, std::string &title)
{
    // 寻找<title>的位置
    size_t begin = result.find("<title>");
    if (begin == std::string::npos)
    {
        return false;
    }

    // 寻找</title>的位置
    size_t end = result.find("</title>");
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

//for debug
void ShowDoc(const DocInfo_t& doc)
{
    std::cout << "title:" << doc.title << std::endl;
    std::cout << "content:" << doc.content << std::endl;
    std::cout << "url:" << doc.url << std::endl;
}