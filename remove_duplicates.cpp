#include <set>
#include <algorithm>
#include "remove_duplicates.h"

using namespace std;

template<typename StringKeyMap>
set<string> GetKeySet(const StringKeyMap& m) {
    set<string> keys;
    for (const auto& item: m) {
        keys.insert({item.first.begin(), item.first.end()});
    }
    return keys;
}

template<typename StringKeyMap>
bool AreEqualByKeys(const StringKeyMap& lhs, const StringKeyMap& rhs) {
    if (lhs.size() != rhs.size()) {
        return false;
    }
    return GetKeySet(lhs) == GetKeySet(rhs);
}

void RemoveDuplicates(SearchServer& search_server) {
    set<int> duplicates;

    for (auto lhs = search_server.begin(); lhs != search_server.end(); ++lhs) {
        for (auto rhs = next(lhs, 1); rhs != search_server.end(); ++rhs) {
            if (AreEqualByKeys(search_server.GetWordFrequencies(*lhs), search_server.GetWordFrequencies(*rhs))) {
                duplicates.insert(*rhs);
            }
        }
    }

    for (const int id: duplicates) {
        cout << "Found duplicate document id " << id << endl;
        search_server.RemoveDocument(id);
    }
}
