#include "requestqueue.h"

using namespace std;

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentStatus status) {
    vector<Document> result = search_server_.FindTopDocuments(raw_query, status);
    UpdateRequests({raw_query, result.empty()});
    return result;
}

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query) {
    vector<Document> result = search_server_.FindTopDocuments(raw_query);
    UpdateRequests({raw_query, result.empty()});
    return result;
}

int RequestQueue::GetNoResultRequests() const {
    return no_result_requests_;
}
