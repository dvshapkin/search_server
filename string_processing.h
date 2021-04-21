#pragma once

#include <string>
#include <vector>
#include <set>

std::vector<std::string> SplitIntoWords(const std::string_view text);

template <typename StringContainer>
std::set<std::string> MakeUniqueNonEmptyStrings(const StringContainer& strings) {
    std::set<std::string> non_empty_strings;
    for (const std::string& str : strings) {
        if (!str.empty()) {
            non_empty_strings.insert(std::move(str));
        }
    }
    return non_empty_strings;
}
