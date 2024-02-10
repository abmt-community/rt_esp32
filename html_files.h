#include <map>
#include <string_view>

struct esp_html_file{
    const char* p;
    size_t s;
    template<int N>
    esp_html_file(const char (&arr)[N]){
        p = arr;
        s = N;
    }
};

extern std::map<std::string_view, esp_html_file > html_files;