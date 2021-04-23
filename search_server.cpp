#include "search_server.h"

using namespace std;

void SearchServer::AddDocument(int document_id, const std::string_view document, DocumentStatus status, const std::vector<int>& ratings) {
    if ((document_id < 0) || (documents_.count(document_id) > 0)) {
        throw invalid_argument("Invalid document_id"s);
    }
    const auto words = SplitIntoWordsNoStop(document);

    const double inv_word_count = 1.0 / words.size();
    for (const string_view word : words) {
        std::string str_word{word};
        if (document_to_word_freqs_[document_id].count(word) == 0) {
            document_to_word_freqs_[document_id][str_word] = 0;
        }
        document_to_word_freqs_[document_id][str_word] += inv_word_count;
    }

    documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status});
    document_ids_.insert(document_id);
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string_view raw_query, const DocumentStatus& status) const {
    return FindTopDocuments(raw_query, [status](int, DocumentStatus document_status, int) {
        return document_status == status;
    });
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string_view raw_query) const {
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

int SearchServer::GetDocumentCount() const {
    return document_ids_.size();
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::string_view raw_query, int document_id) const {
    return MatchDocument(std::execution::seq, raw_query, document_id);
}

SearchServer::const_iterator SearchServer::begin() const {
    return document_ids_.begin();
}

SearchServer::const_iterator SearchServer::end() const {
    return document_ids_.end();
}

const std::map<const std::string_view, double> SearchServer::GetWordFrequencies(int document_id) const {
    std::map<const std::string_view, double> result;
    for (const auto& item: document_to_word_freqs_.at(document_id)) {
        result[item.first] = item.second;
    }
    return result;
}

void SearchServer::RemoveDocument(int document_id) {
    document_ids_.erase(document_id);
    documents_.erase(document_id);
    document_to_word_freqs_.erase(document_id);
}

void SearchServer::RemoveDocument(const std::execution::sequenced_policy &policy, int document_id) {
    RemoveDocument(document_id);
}

void SearchServer::RemoveDocument(const std::execution::parallel_policy &policy, int document_id) {
    // В моем случае здесь нечего "параллелить"
    RemoveDocument(document_id);
}
