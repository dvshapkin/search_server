#pragma once

#include <vector>
#include <deque>

#include "searchserver.h"
#include "document.h"

class RequestQueue {
public:
    explicit RequestQueue(const SearchServer& search_server)
        : search_server_(search_server) {
    }

    // сделаем "обёртки" для всех методов поиска, чтобы сохранять результаты для нашей статистики
    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string& raw_query, const DocumentPredicate& document_predicate) {
        std::vector<Document> result = search_server_.FindTopDocuments(raw_query, document_predicate);
        UpdateRequests({raw_query, result.empty()});
        return result;
    }

    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status);

    std::vector<Document> AddFindRequest(const std::string& raw_query);

    int GetNoResultRequests() const;

private:
    struct QueryResult {
        std::string query;
        bool empty;
    };
    std::deque<QueryResult> requests_;
    const static int sec_in_day_ = 1440;
    // возможно, здесь вам понадобится что-то ещё
    const SearchServer& search_server_;
    int no_result_requests_ = 0;

    void UpdateRequests(const QueryResult& result) {
        requests_.push_back(result);
        if (result.empty) {
            ++no_result_requests_;
        }
        RemoveOutdatedRequests();
    }

    void RemoveOutdatedRequests() {
        int64_t need_to_remove = static_cast<int64_t>(requests_.size()) - sec_in_day_;
        while (need_to_remove > 0) {
            if (requests_.front().empty) {
                --no_result_requests_;
            }
            requests_.pop_front();
            --need_to_remove;
        }
    }
};
