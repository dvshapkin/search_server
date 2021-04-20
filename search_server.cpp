#include "search_server.h"

using namespace std;

void SearchServer::AddDocument(int document_id, const std::string& document, DocumentStatus status, const std::vector<int>& ratings) {
    if ((document_id < 0) || (documents_.count(document_id) > 0)) {
        throw invalid_argument("Invalid document_id"s);
    }
    const auto words = SplitIntoWordsNoStop(document);

    const double inv_word_count = 1.0 / words.size();
    for (const string& word : words) {
        if (document_to_word_freqs_[document_id].count(word) == 0) {
            document_to_word_freqs_[document_id][word] = 0;
        }
        document_to_word_freqs_[document_id][word] += inv_word_count;
    }
    documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status});

    document_ids_.insert(document_id);
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query, const DocumentStatus& status) const {
    return FindTopDocuments(raw_query, [status](int, DocumentStatus document_status, int) {
        return document_status == status;
    });
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query) const {
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

int SearchServer::GetDocumentCount() const {
    return document_ids_.size();
}

std::tuple<std::vector<std::string>, DocumentStatus> SearchServer::MatchDocument(const std::string& raw_query, int document_id) const {
    const auto query = ParseQuery(raw_query);

    vector<string> matched_words;

    for (const string& word : query.plus_words) {
        if (document_to_word_freqs_.count(document_id) == 0) {
            continue;
        }
        if (document_to_word_freqs_.at(document_id).count(word)) {
            matched_words.push_back(word);
        }
    }
    for (const string& word : query.minus_words) {
        if (document_to_word_freqs_.count(document_id) == 0) {
            continue;
        }
        if (document_to_word_freqs_.at(document_id).count(word)) {
            matched_words.clear();
            break;
        }
    }
    return {matched_words, documents_.at(document_id).status};
}

SearchServer::const_iterator SearchServer::begin() const {
    return document_ids_.begin();
}

SearchServer::const_iterator SearchServer::end() const {
    return document_ids_.end();
}

const std::map<std::string, double>& SearchServer::GetWordFrequencies(int document_id) const {
    return document_to_word_freqs_.at(document_id);
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
    // Удаление из document_ids_
    {
        std::vector<int> v(document_ids_.size());
        std::move(policy, document_ids_.begin(), document_ids_.end(), v.begin());
        std::remove(policy, v.begin(), v.end(), document_id);

        std::set<int> temp(v.begin(), v.end());
        document_ids_.swap(temp);
    }

    // Удаление из documents_
    {
        std::vector<std::pair<int, DocumentData>> v(documents_.size());
        std::move(policy, documents_.begin(), documents_.end(), v.begin());
        std::remove_if(policy, v.begin(), v.end(),
                       [document_id](const auto &item) { return item.first == document_id; });

        std::map<int, DocumentData> temp(v.begin(), v.end());
        documents_.swap(temp);
    }

    // Удаление из document_to_word_freqs_
    {
        std::vector<std::pair<int, std::map<std::string, double>>> v(document_to_word_freqs_.size());
        std::move(policy, document_to_word_freqs_.begin(), document_to_word_freqs_.end(), v.begin());
        std::remove_if(policy, v.begin(), v.end(),
                       [document_id](const auto &item) { return item.first == document_id; });

        std::map<int, std::map<std::string, double>> temp(v.begin(), v.end());
        document_to_word_freqs_.swap(temp);
    }

    //std::remove(std::execution::par, document_ids_.begin(), document_ids_.end(), document_id);
    //std::remove(std::execution::par, documents_.begin(), documents_.end(), document_id);
    //std::remove(std::move(policy), document_to_word_freqs_.begin(), document_to_word_freqs_.end(), document_id);
}
