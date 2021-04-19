#include "process_queries.h"
#include <algorithm>
#include <execution>

std::vector<std::vector<Document>> ProcessQueries(
        const SearchServer &search_server,
        const std::vector<std::string> &queries) {
    std::vector<std::vector<Document>> result(queries.size());
    if (queries.size()) {
        std::transform(std::execution::par, queries.cbegin(), queries.cend(), result.begin(),
                       [&search_server](const std::string &query) {
                           return search_server.FindTopDocuments(query);
                       });
    }
    return result;
}
