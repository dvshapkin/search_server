#include "search_server.h"
#include "request_queue.h"
#include "test_example_functions.h"
#include "remove_duplicates.h"
#include "paginator.h"
#include "process_queries.h"

#include <iostream>
#include <string>
#include <vector>
#include <execution>

#include "test_parallel_work.h"

using namespace std;

void Test0() {
    SearchServer search_server("and in on"s);
    AddDocument(search_server, 0, ""s, DocumentStatus::ACTUAL, {7, 2, 7});
    AddDocument(search_server, 1, "in"s, DocumentStatus::ACTUAL, {7, 2, 7});
    AddDocument(search_server, 2, "fluffy"s, DocumentStatus::ACTUAL, {7, 2, 7});
    AddDocument(search_server, 3, "white cat and fancy collar"s, DocumentStatus::ACTUAL, {7, 2, 7});
    AddDocument(search_server, 4, "fluffy cat fluffy tail"s, DocumentStatus::ACTUAL, {7, 2, 7});
    AddDocument(search_server, 5, "  soigne   dog expressive eyes  "s, DocumentStatus::ACTUAL, {7, 2, 7});

    const string query = "fluffy soigne cat -tail"s;
    for (auto now : search_server.FindTopDocuments(query)) {
        cout << "{ document_id = " << now.id << ", relevance = "
             << now.relevance << " }" << endl;
    }

    const string query2 = "fluffy soigne cat -tail"s;
    const auto[words, status] = search_server.MatchDocument(query2, 4);
    cout << words.size() << " words for document 4"s << endl;
}

void Test1() {
    SearchServer search_server("and with"s);

    AddDocument(search_server, 1, "funny pet and nasty rat"s, DocumentStatus::ACTUAL, {7, 2, 7});
    AddDocument(search_server, 2, "funny pet with curly hair"s, DocumentStatus::ACTUAL, {1, 2});

    // дубликат документа 2, будет удалён
    AddDocument(search_server, 3, "funny pet with curly hair"s, DocumentStatus::ACTUAL, {1, 2});

    // отличие только в стоп-словах, считаем дубликатом
    AddDocument(search_server, 4, "funny pet and curly hair"s, DocumentStatus::ACTUAL, {1, 2});

    // множество слов такое же, считаем дубликатом документа 1
    AddDocument(search_server, 5, "funny funny pet and nasty nasty rat"s, DocumentStatus::ACTUAL, {1, 2});

    // добавились новые слова, дубликатом не является
    AddDocument(search_server, 6, "funny pet and not very nasty rat"s, DocumentStatus::ACTUAL, {1, 2});

    // множество слов такое же, как в id 6, несмотря на другой порядок, считаем дубликатом
    AddDocument(search_server, 7, "very nasty rat and not very funny pet"s, DocumentStatus::ACTUAL, {1, 2});

    // есть не все слова, не является дубликатом
    AddDocument(search_server, 8, "pet with rat and rat and rat"s, DocumentStatus::ACTUAL, {1, 2});

    // слова из разных документов, не является дубликатом
    AddDocument(search_server, 9, "nasty rat with curly hair"s, DocumentStatus::ACTUAL, {1, 2});

    cout << "Before duplicates removed: "s << search_server.GetDocumentCount() << endl;
    RemoveDuplicates(search_server);
    cout << "After duplicates removed: "s << search_server.GetDocumentCount() << endl;
}

void Test2() {
    SearchServer search_server("and with"s);

    int id = 0;
    for (
        const string &text : {
            "funny pet and nasty rat"s,
            "funny pet with curly hair"s,
            "funny pet and not very nasty rat"s,
            "pet with rat and rat and rat"s,
            "nasty rat with curly hair"s,
    }
            ) {
        search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, {1, 2});
    }

    const vector<string> queries = {
            "nasty rat -not"s,
            "not very funny nasty pet"s,
            "curly hair"s
    };
    id = 0;
    for (
        const auto &documents : ProcessQueries(search_server, queries)
            ) {
        cout << documents.size() << " documents for query ["s << queries[id++] << "]"s << endl;
    }
}

void Test3() {
    SearchServer search_server("and with"s);

    int id = 0;
    for (
        const string &text : {
            "funny pet and nasty rat"s,
            "funny pet with curly hair"s,
            "funny pet and not very nasty rat"s,
            "pet with rat and rat and rat"s,
            "nasty rat with curly hair"s,
    }
            ) {
        search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, {1, 2});
    }

    const vector<string> queries = {
            "nasty rat -not"s,
            "not very funny nasty pet"s,
            "curly hair"s
    };
    for (const Document &document : ProcessQueriesJoined(search_server, queries)) {
        cout << "Document "s << document.id << " matched with relevance "s << document.relevance << endl;
    }
}

void Test4() {
    SearchServer search_server("and with"s);

    int id = 0;
    for (
        const string &text : {
            "funny pet and nasty rat"s,
            "funny pet with curly hair"s,
            "funny pet and not very nasty rat"s,
            "pet with rat and rat and rat"s,
            "nasty rat with curly hair"s,
    }
            ) {
        search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, {1, 2});
    }

    const string query = "curly and funny"s;

    auto report = [&search_server, &query] {
        cout << search_server.GetDocumentCount() << " documents total, "s
             << search_server.FindTopDocuments(query).size() << " documents for query ["s << query << "]"s << endl;
    };

    report();
    // однопоточная версия
    search_server.RemoveDocument(5);
    report();
    // однопоточная версия
    search_server.RemoveDocument(std::execution::seq, 1);
    report();
    // многопоточная версия
    search_server.RemoveDocument(std::execution::par, 2);
    report();

}

void Test5() {
    SearchServer search_server("and with"s);

    int id = 0;
    for (
        const string &text : {
            "funny pet and nasty rat"s,
            "funny pet with curly hair"s,
            "funny pet and not very nasty rat"s,
            "pet with rat and rat and rat"s,
            "nasty rat with curly hair"s,
    }
            ) {
        search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, {1, 2});
    }

    const string query = "curly and funny -not"s;

    {
        const auto[words, status] = search_server.MatchDocument(query, 1);
        cout << words.size() << " words for document 1"s << endl;
        // 1 words for document 1
    }

    {
        const auto[words, status] = search_server.MatchDocument(execution::seq, query, 2);
        cout << words.size() << " words for document 2"s << endl;
        // 2 words for document 2
    }

    {
        const auto[words, status] = search_server.MatchDocument(execution::par, query, 3);
        cout << words.size() << " words for document 3"s << endl;
        // 0 words for document 3
    }
}

void Test6() {
    SearchServer search_server("and with"s);

    int id = 0;
    for (
        const string &text : {
            "white cat and yellow hat"s,
            "curly cat curly tail"s,
            "nasty dog with big eyes"s,
            "nasty pigeon john"s,
    }
            ) {
        search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, {1, 2});
    }

    cout << "ACTUAL by default:"s << endl;
    // последовательная версия
    for (const Document &document : search_server.FindTopDocuments("curly nasty cat"s)) {
        PrintDocument(document);
    }
    cout << "BANNED:"s << endl;
    // последовательная версия
    for (const Document &document : search_server.FindTopDocuments(execution::seq, "curly nasty cat"s,
                                                                   DocumentStatus::BANNED)) {
        PrintDocument(document);
    }

    cout << "Even ids:"s << endl;
    // параллельная версия
    for (const Document &document : search_server.FindTopDocuments(execution::par, "curly nasty cat"s,
                                                                   [](int document_id, DocumentStatus status,
                                                                      int rating) { return document_id % 2 == 0; })) {
        PrintDocument(document);
    }

}

int main() {

    Test0();
    Test1();
    Test2();
    Test3();
    Test4();
    Test5();
    Test6();

    return 0;
}
