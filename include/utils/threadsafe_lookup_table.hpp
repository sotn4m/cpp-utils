#pragma once
#include <functional>
#include <list>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <stdexcept>
#include <utility>
#include <vector>

namespace utils {

template <typename Key, typename Value, typename Hash = std::hash<Key>>
class threadsafe_lookup_table {
 public:
  using key_type = Key;
  using value_type = Value;
  using hash_type = Hash;
  static_assert (std::is_invocable_r_v<std::size_t, Hash, Key const&>);
  static_assert (std::equality_comparable<Key>);

  explicit threadsafe_lookup_table (std::size_t num_buckets = 19,
                                    Hash const& hasher = Hash ())
      : buckets (num_buckets), hasher (hasher) {
    if (num_buckets < 1) {
      throw std::invalid_argument ("num_buckets must be >= 1");
    }
  }
  threadsafe_lookup_table (threadsafe_lookup_table const& other) = delete;
  threadsafe_lookup_table& operator= (threadsafe_lookup_table const& other) =
      delete;

  std::optional<Value> value_for (Key const& key) const {
    return get_bucket (key).value_for (key);
  }

  void add_or_update_mapping (Key const& key, Value const& value) {
    get_bucket (key).add_or_update_mapping (key, value);
  }

  void add_or_update_mapping (Key&& key, Value&& value) {
    get_bucket (key).add_or_update_mapping (std::move (key), std::move (value));
  }

  void remove_mapping (Key const& key) {
    get_bucket (key).remove_mapping (key);
  }

 private:
  class bucket_type {
   public:
    std::optional<Value> value_for (Key const& key) const {
      std::shared_lock lock (mutex);
      const auto found_entry = find_entry_for (key);
      if (found_entry == data.end ()) {
        return std::nullopt;
      }
      return found_entry->second;
    }

    void add_or_update_mapping (Key&& key, Value&& value) {
      std::unique_lock lock (mutex);
      auto found_entry = find_entry_for (key);
      if (found_entry == data.end ()) {
        data.push_back (bucket_value (std::move (key), std::move (value)));
      } else {
        found_entry->second = std::move (value);
      }
    }

    void add_or_update_mapping (Key const& key, Value const& value) {
      std::unique_lock lock (mutex);
      auto found_entry = find_entry_for (key);
      if (found_entry == data.end ()) {
        data.push_back (bucket_value (key, value));
      } else {
        found_entry->second = value;
      }
    }

    void remove_mapping (Key const& key) {
      std::unique_lock lock (mutex);
      const auto found_entry = find_entry_for (key);
      if (found_entry != data.end ()) {
        data.erase (found_entry);
      }
    }

   private:
    using bucket_value = std::pair<Key, Value>;
    using bucket_data = std::list<bucket_value>;
    using const_bucket_iterator = bucket_data::const_iterator;
    using bucket_iterator = bucket_data::iterator;

    bucket_data data;
    mutable std::shared_mutex mutex;

    bucket_iterator find_entry_for (Key const& key) {
      return std::ranges::find_if (
          data, [&] (bucket_value const& item) { return item.first == key; });
    }

    const_bucket_iterator find_entry_for (Key const& key) const {
      return std::ranges::find_if (
          data, [&] (bucket_value const& item) { return item.first == key; });
    }
  };

  std::vector<bucket_type> buckets;
  Hash hasher;

  bucket_type& get_bucket (Key const& key) {
    std::size_t const bucket_index = hasher (key) % buckets.size ();
    return buckets[bucket_index];
  }

  bucket_type const& get_bucket (Key const& key) const {
    std::size_t const bucket_index = hasher (key) % buckets.size ();
    return buckets[bucket_index];
  }
};
}  // namespace utils
