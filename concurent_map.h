#include <cstdlib>
#include <cassert>
#include <future>
#include <map>
#include <string>
#include <vector>

using namespace std::string_literals;

template<typename Key, typename Value>
class ConcurrentMap {
public:
    static_assert(std::is_integral_v<Key>, "ConcurrentMap supports only integer keys"s);

    struct Access {
    private:
        std::lock_guard<std::mutex> guard_;
    public:
        Value &ref_to_value;

        Access(std::map<Key, Value> &bucket, const Key &key, std::mutex &m)
                : guard_(m), ref_to_value(bucket[key]) {
        }
    };

    explicit ConcurrentMap(size_t bucket_count)
            : buckets_(bucket_count), vm_(bucket_count) {
        assert(bucket_count > 0);
    }

    Access operator[](const Key &key) {
        size_t bucket_index = static_cast<size_t>(key) % buckets_.size();
        return Access{buckets_[bucket_index], key, vm_[bucket_index]};
    }

    std::map<Key, Value> BuildOrdinaryMap() {
        std::map<Key, Value> ordinary_map;
        for (size_t i = 0; i < buckets_.size(); ++i) {
            vm_[i].lock();
            ordinary_map.insert(buckets_[i].cbegin(), buckets_[i].cend());
            vm_[i].unlock();
        }
        return ordinary_map;
    }

private:
    std::vector<std::map<Key, Value>> buckets_;
    std::vector<std::mutex> vm_;
};
