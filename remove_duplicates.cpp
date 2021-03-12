#include <set>
#include <algorithm>
#include "remove_duplicates.h"

using namespace std;

void RemoveDuplicates(SearchServer& search_server) {
    set<int> duplicates;

    for (auto lhs = search_server.begin(); lhs != search_server.end(); ++lhs) {
        const auto lhs_words = search_server.GetWordFrequencies(*lhs);
        for (auto rhs = next(lhs, 1); rhs != search_server.end(); ++rhs) {
            const auto rhs_words = search_server.GetWordFrequencies(*rhs);
            if (lhs_words == rhs_words) {
                duplicates.insert(*rhs);
            }
        }
    }

//    auto new_end_it = unique(search_server.begin(), search_server.end(), [&search_server](const auto &lhs, const auto &rhs){
//        return search_server.GetWordFrequencies(lhs) == search_server.GetWordFrequencies(rhs);
//    });

//    for (auto it = new_end_it; it != search_server.end(); ++it) {
//        cout << "Found duplicate document id " << *it << endl;
//        duplicates.push_back(*it);
//    }

    for (const int id: duplicates) {
        cout << "Found duplicate document id " << id << endl;
        search_server.RemoveDocument(id);
    }
}
