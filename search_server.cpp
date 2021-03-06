#include "search_server.h"

using namespace std;

void SearchServer::AddDocument(int document_id, const std::string_view document, DocumentStatus status, const std::vector<int>& ratings) {
    if ((document_id < 0) || (documents_.count(document_id) > 0)) {
        throw invalid_argument("Invalid document_id"s);
    }
    const auto words = SplitIntoWordsNoStop(document);

    const double inv_word_count = 1.0 / words.size();
    for (const string_view word : words) {
        const auto insert_result = words_.insert(std::string {word});
        const std::string_view word_view {*insert_result.first};

        if (document_to_word_freqs_[document_id].count(word_view) == 0) {
            document_to_word_freqs_[document_id][word_view] = 0;
        }
        document_to_word_freqs_[document_id][word_view] += inv_word_count;

        word_to_document_freqs_[word_view].insert(document_id);
    }

    documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status});
    document_ids_.insert(document_id);
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string_view raw_query, const DocumentStatus& status) const {
    return FindTopDocuments(std::execution::seq, raw_query, status);
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string_view raw_query) const {
    return FindTopDocuments(std::execution::seq, raw_query);
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

    for (auto& item: word_to_document_freqs_) {
        if (item.second.count(document_id)) {
            item.second.erase(document_id);
        }
    }
}

void SearchServer::RemoveDocument(const std::execution::sequenced_policy &policy, int document_id) {
    RemoveDocument(document_id);
}

void SearchServer::RemoveDocument(const std::execution::parallel_policy &policy, int document_id) {
    RemoveDocument(document_id);
}
