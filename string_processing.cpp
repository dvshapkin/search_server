#include "string_processing.h"

using namespace std;

std::vector<std::string_view> SplitIntoWords(const std::string_view text) {
    vector<string_view> words;

    std::size_t pos_start = 0;
    std::size_t pos_end = 0;
    while (pos_start != std::string_view::npos) {
        pos_start = text.find_first_not_of(' ', pos_start);
        if (pos_start == std::string_view::npos) { break; }
        pos_end = pos_start;
        pos_end = text.find_first_of(' ', pos_end);
        if (pos_start != pos_end) {
            words.push_back({&text[pos_start],
                             (pos_end == std::string_view::npos) ? text.size() - pos_start : pos_end - pos_start});
        }
        pos_start = pos_end;
    }

    return words;
}
