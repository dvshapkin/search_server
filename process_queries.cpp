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


std::vector<Document> ProcessQueriesJoined(
        const SearchServer &search_server,
        const std::vector<std::string> &queries) {
    std::vector<Document> result;
    auto vec_of_vec = ProcessQueries(search_server, queries);
    if (vec_of_vec.size()) {
        result.reserve(vec_of_vec.size() * vec_of_vec[0].size());
        for (const auto &v: vec_of_vec) {
            for (const Document &doc: v) {
                result.push_back(doc);
            }
        }
    }
    return result;
}
