#include "search.hpp"
const std::string output_path = "/home/wr/boost_search/data/raw_doc/raw.txt";

int main()
{
    ns_search::Search* search = new ns_search::Search();
    if (!search->InitSearch(output_path))
        return 0;

    while(true)
    {
        std::string word;
        std::cout << "Please input your key word# " << std::endl;
        std::cin >> word;

        std::string json_string;
        search->SearchForKeyWord(word, &json_string);
        std::cout << json_string << std::endl;
    }

    return 0;
}