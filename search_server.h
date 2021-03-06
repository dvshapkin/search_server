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
#include <mutex>
#include <type_traits>

#include "read_input_functions.h"
#include "document.h"
#include "string_processing.h"
#include "concurrent_map.h"

#include "log_duration.h"

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
    AddDocument(int document_id, std::string_view document, DocumentStatus status,
                const std::vector<int> &ratings);

    template<typename ExecutionPolicy, typename DocumentPredicate>
    std::vector<Document>
    FindTopDocuments(ExecutionPolicy &&policy, const std::string_view raw_query,
                     const DocumentPredicate &document_predicate) const {
        const auto query = ParseQuery(raw_query);

        auto matched_documents = FindAllDocuments(std::execution::seq, query, document_predicate);

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

    template<typename DocumentPredicate>
    std::vector<Document>
    FindTopDocuments(const std::string_view raw_query,
                     const DocumentPredicate &document_predicate) const {
        return FindTopDocuments(std::execution::seq, raw_query, document_predicate);
    }

    template<typename ExecutionPolicy>
    std::vector<Document>
    FindTopDocuments(ExecutionPolicy &&policy, const std::string_view raw_query, const DocumentStatus &status) const {
        return FindTopDocuments(policy, raw_query, [status](int, DocumentStatus document_status, int) {
            return document_status == status;
        });
    }

    [[nodiscard]] std::vector<Document>
    FindTopDocuments(std::string_view raw_query, const DocumentStatus &status) const;

    template<typename ExecutionPolicy>
    [[nodiscard]] std::vector<Document>
    FindTopDocuments(ExecutionPolicy &&policy, const std::string_view raw_query) const {
        return FindTopDocuments(policy, raw_query, DocumentStatus::ACTUAL);
    }

    [[nodiscard]] std::vector<Document> FindTopDocuments(std::string_view raw_query) const;

    [[nodiscard]] int GetDocumentCount() const;

    [[nodiscard]] std::tuple<std::vector<std::string_view>, DocumentStatus>
    MatchDocument(std::string_view raw_query, int document_id) const;

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
        if (!hase_minus_words) {
            std::for_each(policy, query.plus_words.cbegin(), query.plus_words.cend(), [&](const auto &word) {
                if (document_to_word_freqs_.count(document_id)) {
                    if (document_to_word_freqs_.at(document_id).count(word)) {
                        matched_words.push_back(word);
                    }
                }
            });
        }

        return {matched_words, documents_.at(document_id).status};
    }

    [[nodiscard]] SearchServer::const_iterator begin() const;

    [[nodiscard]] SearchServer::const_iterator end() const;

    [[nodiscard]] const std::map<const std::string_view, double> GetWordFrequencies(int document_id) const;

    void RemoveDocument(int document_id);

    void RemoveDocument(const std::execution::sequenced_policy &policy, int document_id);

    void RemoveDocument(const std::execution::parallel_policy &policy, int document_id);


private:
    const std::set<std::string, std::less<>> stop_words_;
    std::set<std::string, std::less<>> words_;
    std::map<int, std::map<std::string_view, double, std::less<>>> document_to_word_freqs_;
    std::map<std::string_view, std::set<int>> word_to_document_freqs_;
    std::map<int, DocumentData> documents_;
    std::set<int> document_ids_;


    [[nodiscard]] bool IsStopWord(const std::string_view &word) const {
        return stop_words_.count(word) > 0;
    }

    static bool IsValidWord(const std::string_view &word) {
        // A valid word must not contain special characters
        return std::none_of(word.begin(), word.end(), [](char c) {
            return c >= '\0' && c < ' ';
        });
    }

    [[nodiscard]] std::vector<std::string_view> SplitIntoWordsNoStop(const std::string_view text) const {
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
        std::string_view data;
        bool is_minus;
        bool is_stop;
    };

    [[nodiscard]] QueryWord ParseQueryWord(const std::string_view &text) const {
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

        return {word, is_minus, IsStopWord(word)};
    }

    struct Query {
        std::set<std::string_view> plus_words;
        std::set<std::string_view> minus_words;
    };

    [[nodiscard]] Query ParseQuery(const std::string_view text) const {
        Query result;
        const auto words = SplitIntoWords(text);
        std::for_each(words.cbegin(), words.cend(), [&](const auto &word) {
            const auto query_word = ParseQueryWord(word);
            if (!query_word.is_stop) {
                if (query_word.is_minus) {
                    result.minus_words.insert(query_word.data);
                } else {
                    result.plus_words.insert(query_word.data);
                }
            }
        });
        return result;
    }

    // Existence required
    [[nodiscard]] double ComputeWordInverseDocumentFreq(const std::string_view &word) const {
        size_t doc_count = 0;
        if (word_to_document_freqs_.count(word)) {
            doc_count = word_to_document_freqs_.at(word).size();
        }
        return std::log(GetDocumentCount() * 1.0 / doc_count);
    }

    template<typename DocumentPredicate>
    std::vector<Document>
    FindAllDocumentsSequenced(const Query &query, const DocumentPredicate &document_predicate) const {

        std::map<int, double> document_to_relevance;

        for (const std::string_view word : query.plus_words) {
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto &item : document_to_word_freqs_) {
                const auto document_id = item.first;
                const auto &word_freqs = item.second;
                if (std::any_of(query.minus_words.cbegin(), query.minus_words.cend(),
                                [&word_freqs](const auto &word) { return word_freqs.count(word); })
                    || word_freqs.count(word) == 0) {
                    continue;
                }
                const auto &document_data = documents_.at(document_id);
                if (document_predicate(document_id, document_data.status, document_data.rating)) {
                    document_to_relevance[document_id] += word_freqs.at(word) * inverse_document_freq;
                }
            }
        }

        std::vector<Document> matched_documents;
        matched_documents.reserve(document_to_relevance.size());
        for (const auto[document_id, relevance] : document_to_relevance) {
            matched_documents.emplace_back(document_id, relevance, documents_.at(document_id).rating);
        }

        return matched_documents;
    }

    template<typename DocumentPredicate>
    std::vector<Document>
    FindAllDocumentsParallel(const Query &query, const DocumentPredicate &document_predicate) const {

        // ???????????? ?????????????????? ?? ??????????-??????????????
        std::set<int> docs_with_minus_word;
        std::mutex m1;
        auto begin = query.minus_words.cbegin();
        auto end = query.minus_words.cend();
        std::for_each(std::execution::par, document_to_word_freqs_.cbegin(), document_to_word_freqs_.cend(),
                      [begin, end, &docs_with_minus_word, &m1](const auto &item) {
                          const auto document_id = item.first;
                          const auto &word_freqs = item.second;
                          if (std::any_of(begin, end, [&word_freqs, document_id, &docs_with_minus_word](
                                  const auto &word) { return word_freqs.count(word); })) {
                              m1.lock();
                              docs_with_minus_word.insert(document_id);
                              m1.unlock();
                          }
                      });

        std::unordered_map<std::string_view, double> inverse_document_freq;
        for (const auto& word: query.plus_words) {
            inverse_document_freq[word] = ComputeWordInverseDocumentFreq(word);
        }

        ConcurrentMap<int, double> document_to_relevance_concurrent(8);
        const auto &docs = documents_;
        std::for_each(std::execution::par, document_to_word_freqs_.cbegin(), document_to_word_freqs_.cend(),
                      [&docs_with_minus_word, &docs, &inverse_document_freq, document_predicate, &document_to_relevance_concurrent](const auto &item) {
                          const auto document_id = item.first;
                          const auto &word_freqs = item.second;
                          if (docs_with_minus_word.count(document_id) == 0) {
                              for (const auto& [word, inverse_freq]: inverse_document_freq) {
                                  if (word_freqs.count(word) == 0) { continue; }
                                  const auto &document_data = docs.at(document_id);
                                  if (document_predicate(document_id, document_data.status, document_data.rating)) {
                                      document_to_relevance_concurrent[document_id].ref_to_value += word_freqs.at(word) * inverse_freq;
                                  }
                              }
                          }
                      });

        std::map<int, double> document_to_relevance = document_to_relevance_concurrent.BuildOrdinaryMap();
        std::vector<Document> matched_documents;
        matched_documents.reserve(document_to_relevance.size());
        std::mutex m2;
        std::for_each(std::execution::par, document_to_relevance.cbegin(), document_to_relevance.cend(),
                      [&matched_documents, &m2, &docs](const auto &item) {
                          m2.lock();
                          auto destination = &matched_documents.emplace_back();
                          m2.unlock();
                          const auto document_id = item.first;
                          const auto relevance = item.second;
                          *destination = {document_id, relevance, docs.at(document_id).rating};
                      });

        return matched_documents;
    }

    template<typename ExecutionPolicy, typename DocumentPredicate>
    std::vector<Document>
    FindAllDocuments(ExecutionPolicy &&, const Query &query, const DocumentPredicate &document_predicate) const {
        if constexpr(std::is_same_v<ExecutionPolicy, std::execution::sequenced_policy>) {
            return FindAllDocumentsSequenced(query, document_predicate);
        } else {
            return FindAllDocumentsParallel(query, document_predicate);
        }
    }
};
