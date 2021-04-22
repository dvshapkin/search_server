#pragma once

#include <cmath>
#include <stdexcept>
#include <algorithm>
#include <numeric>
#include <utility>
#include <vector>
#include <map>
#include <set>
#include <execution>

#include "read_input_functions.h"
#include "document.h"
#include "string_processing.h"

using namespace std::literals::string_literals;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

class SearchServer {
private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };
public:
    using const_iterator = std::set<int>::const_iterator;

    template<typename StringContainer>
    explicit SearchServer(const StringContainer &stop_words)
            : stop_words_(MakeUniqueNonEmptyStrings(stop_words))  // Extract non-empty stop words
    {
        if (!all_of(stop_words_.begin(), stop_words_.end(), IsValidWord)) {
            throw std::invalid_argument("Some of stop words are invalid"s);
        }
    }

    explicit SearchServer(const std::string_view stop_words_text)
            : SearchServer(SplitIntoWords(stop_words_text))  // Invoke delegating constructor
    {
    }

    explicit SearchServer(const std::string &stop_words_text)
            : SearchServer(std::string_view(stop_words_text))  // Invoke delegating constructor
    {
    }

    void
    AddDocument(int document_id, const std::string_view document, DocumentStatus status,
                const std::vector<int> &ratings);

    template<typename DocumentPredicate>
    std::vector<Document>
    FindTopDocuments(const std::string_view raw_query, const DocumentPredicate &document_predicate) const {
        const auto query = ParseQuery(raw_query);

        auto matched_documents = FindAllDocuments(query, document_predicate);

        std::sort(matched_documents.begin(), matched_documents.end(), [](const Document &lhs, const Document &rhs) {
            const double EPSILON = 1e-6;
            if (std::abs(lhs.relevance - rhs.relevance) < EPSILON) {
                return lhs.rating > rhs.rating;
            } else {
                return lhs.relevance > rhs.relevance;
            }
        });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }

        return matched_documents;
    }

    std::vector<Document> FindTopDocuments(const std::string_view raw_query, const DocumentStatus &status) const;

    std::vector<Document> FindTopDocuments(const std::string_view raw_query) const;

    int GetDocumentCount() const;

    std::tuple<std::vector<std::string_view>, DocumentStatus>
    MatchDocument(const std::string_view raw_query, int document_id) const;

//    std::tuple<std::vector<std::string_view>, DocumentStatus>
//    MatchDocument(const std::execution::sequenced_policy &policy, const std::string_view raw_query, int document_id) const;

//    std::tuple<std::vector<std::string_view>, DocumentStatus>
//    MatchDocument(const std::execution::parallel_policy &policy, const std::string_view raw_query, int document_id) const;
    template<typename ExecutionPolicy>
    std::tuple<std::vector<std::string_view>, DocumentStatus>
    MatchDocument(ExecutionPolicy &&policy, const std::string_view raw_query, int document_id) const {
        const auto query = ParseQuery(raw_query);

        std::vector<std::string_view> matched_words;

        bool hase_minus_words = std::any_of(policy, query.minus_words.cbegin(), query.minus_words.cend(),
                                            [&](const auto &word) {
                                                return document_to_word_freqs_.count(document_id) &&
                                                       document_to_word_freqs_.at(document_id).count(word);
                                            });
        if (hase_minus_words) {
            return {matched_words, documents_.at(document_id).status};
        }

        std::for_each(policy, query.plus_words.cbegin(), query.plus_words.cend(), [&](const auto &word) {
            if (document_to_word_freqs_.count(document_id)) {
                if (document_to_word_freqs_.at(document_id).count(word)) {
                    matched_words.push_back(word);
                }
            }
        });

        return {matched_words, documents_.at(document_id).status};
    }

    SearchServer::const_iterator begin() const;

    SearchServer::const_iterator end() const;

    const std::map<const std::string_view, double> GetWordFrequencies(int document_id) const;

    void RemoveDocument(int document_id);

    void RemoveDocument(const std::execution::sequenced_policy &policy, int document_id);

    void RemoveDocument(const std::execution::parallel_policy &policy, int document_id);

private:
    const std::set<std::string, std::less<>> stop_words_;
    std::map<int, std::map<std::string, double>> document_to_word_freqs_;
    std::map<int, DocumentData> documents_;
    std::set<int> document_ids_;

    bool IsStopWord(const std::string_view &word) const {
        return stop_words_.count(word) > 0;
    }

    static bool IsValidWord(const std::string_view &word) {
        // A valid word must not contain special characters
        return std::none_of(word.begin(), word.end(), [](char c) {
            return c >= '\0' && c < ' ';
        });
    }

    std::vector<std::string_view> SplitIntoWordsNoStop(const std::string_view text) const {
        std::vector<std::string_view> words;
        for (const std::string_view &word : SplitIntoWords(text)) {
            if (!IsValidWord(word)) {
                throw std::invalid_argument("Word "s + std::string{word} + " is invalid"s);
            }
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

    static int ComputeAverageRating(const std::vector<int> &ratings) {
        if (ratings.empty()) {
            return 0;
        }
        int rating_sum = accumulate(ratings.begin(), ratings.end(), 0);
        return rating_sum / static_cast<int>(ratings.size());
    }

    struct QueryWord {
        std::string data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(const std::string_view &text) const {
        if (text.empty()) {
            throw std::invalid_argument("Query word is empty"s);
        }
        std::string_view word = text;
        bool is_minus = false;
        if (word[0] == '-') {
            is_minus = true;
            word = word.substr(1);
        }
        if (word.empty() || word[0] == '-' || !IsValidWord(word)) {
            throw std::invalid_argument("Query word "s + std::string{text} + " is invalid");
        }

        return {std::string{word}, is_minus, IsStopWord(word)};
    }

    struct Query {
        std::set<std::string> plus_words;
        std::set<std::string> minus_words;
    };

    Query ParseQuery(const std::string_view text) const {
        Query result;
//        for (const std::string_view &word : SplitIntoWords(text)) {
//            const auto query_word = ParseQueryWord(word);
//            if (!query_word.is_stop) {
//                if (query_word.is_minus) {
//                    result.minus_words.insert(std::move(query_word.data));
//                } else {
//                    result.plus_words.insert(std::move(query_word.data));
//                }
//            }
//        }
        const auto words = SplitIntoWords(text);
        std::for_each(words.cbegin(), words.cend(), [&](const auto& word){
            const auto query_word = ParseQueryWord(word);
            if (!query_word.is_stop) {
                if (query_word.is_minus) {
                    result.minus_words.insert(std::move(query_word.data));
                } else {
                    result.plus_words.insert(std::move(query_word.data));
                }
            }
        });
        return result;
    }

    // Existence required
    double ComputeWordInverseDocumentFreq(const std::string &word) const {
        int doc_count = count_if(
                document_to_word_freqs_.cbegin(),
                document_to_word_freqs_.cend(),
                [word](const auto item) {
                    return item.second.count(word) > 0;
                });
        return std::log(GetDocumentCount() * 1.0 / doc_count);
    }

    template<typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const Query &query, const DocumentPredicate &document_predicate) const {

        // Сформируем множество документов, содержащих минус слова
        std::set<int> docs_with_minus_words;
        for (const std::string &word : query.minus_words) {
            for (const auto&[document_id, word_freqs] : document_to_word_freqs_) {
                if (word_freqs.count(word) == 0) {
                    continue;
                }
                docs_with_minus_words.insert(document_id);
            }
        }

        std::map<int, double> document_to_relevance;
        for (const std::string &word : query.plus_words) {
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto&[document_id, word_freqs] : document_to_word_freqs_) {
                if (docs_with_minus_words.count(document_id) || word_freqs.count(word) == 0) {
                    continue;
                }
                const auto &document_data = documents_.at(document_id);
                if (document_predicate(document_id, document_data.status, document_data.rating)) {
                    if (document_to_relevance.count(document_id) == 0) {
                        document_to_relevance[document_id] = 0;
                    }
                    document_to_relevance[document_id] += word_freqs.at(word) * inverse_document_freq;
                }
            }
        }

//        for (const std::string &word : query.minus_words) {
//            for (const auto&[document_id, word_freqs] : document_to_word_freqs_) {
//                if (word_freqs.count(word) == 0) {
//                    continue;
//                }
//                document_to_relevance.erase(document_id);
//            }
//        }

        std::vector<Document> matched_documents;
        for (const auto[document_id, relevance] : document_to_relevance) {
            matched_documents.push_back({document_id, relevance, documents_.at(document_id).rating});
        }
        return matched_documents;
    }
};
