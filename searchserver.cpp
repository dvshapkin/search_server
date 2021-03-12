#include "searchserver.h"

using namespace std;

void SearchServer::AddDocument(int document_id, const std::string& document, DocumentStatus status, const std::vector<int>& ratings) {
    if ((document_id < 0) || (documents_.count(document_id) > 0)) {
        throw invalid_argument("Invalid document_id"s);
    }
    const auto words = SplitIntoWordsNoStop(document);

    const double inv_word_count = 1.0 / words.size();
    for (const string& word : words) {
        if (word_to_document_freqs_[word].count(document_id) == 0) {
            word_to_document_freqs_[word][document_id] = 0;
        }
        word_to_document_freqs_[word][document_id] += inv_word_count;

        if (document_to_word_freqs_[document_id].count(word) == 0) {
            document_to_word_freqs_[document_id][word] = 0;
        }
        document_to_word_freqs_[document_id][word] += inv_word_count;
    }
    documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status});
    //document_ids_.push_back(document_id);
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
    return documents_.size();
}

std::tuple<std::vector<std::string>, DocumentStatus> SearchServer::MatchDocument(const std::string& raw_query, int document_id) const {
    const auto query = ParseQuery(raw_query);

    vector<string> matched_words;
//    for (const string& word : query.plus_words) {
//        if (word_to_document_freqs_.count(word) == 0) {
//            continue;
//        }
//        if (word_to_document_freqs_.at(word).count(document_id)) {
//            matched_words.push_back(word);
//        }
//    }
//    for (const string& word : query.minus_words) {
//        if (word_to_document_freqs_.count(word) == 0) {
//            continue;
//        }
//        if (word_to_document_freqs_.at(word).count(document_id)) {
//            matched_words.clear();
//            break;
//        }
//    }
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
    return documents_.begin();
}

SearchServer::const_iterator SearchServer::end() const {
    return documents_.end();
}

const std::map<std::string, double>& SearchServer::GetWordFrequencies(int document_id) const {
    return document_to_word_freqs_.at(document_id);
}

void SearchServer::RemoveDocument(int document_id) {
    documents_.erase(document_id);
    for (auto& [word, id_to_freqs]: word_to_document_freqs_) {
        if (id_to_freqs.count(document_id) > 0) {
            id_to_freqs.erase(document_id);
            if (id_to_freqs.empty()) {
                word_to_document_freqs_.erase(word);
            }
        }
    }
}