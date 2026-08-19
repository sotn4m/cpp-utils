#pragma once
#include <list>
#include <optional>
#include <unordered_map>

namespace utils {

template <typename Key, typename Value>
class lru_cache {
 public:
  explicit lru_cache (std::size_t capacity) : capacity_ {capacity} {}

  auto get (const Key& key) -> std::optional<Value>;
  template <typename U, typename I>
  auto put (U&& key, I&& value) -> void;

  auto size () const -> std::size_t { return cache_.size (); }
  auto contains (const Key& key) const -> bool { return cache_.contains (key); }
  auto erase (const Key& key) -> bool;
  auto clear () -> void {
    list_.clear ();
    cache_.clear ();
  }

 private:
  using ListNode = std::pair<Key, Value>;
  using List = std::list<ListNode>;
  using ListIterator = typename List::iterator;
  using Cache = std::unordered_map<Key, ListIterator>;

  List list_;
  Cache cache_;
  const std::size_t capacity_ {};
};

template <typename Key, typename Value>
auto lru_cache<Key, Value>::get (const Key& key) -> std::optional<Value> {
  if (auto it = cache_.find (key); it != cache_.end ()) {
    // mark the key as most recently used
    list_.splice (list_.begin (), list_, it->second);
    return it->second->second;
  }
  return std::nullopt;
}

template <typename Key, typename Value>
auto lru_cache<Key, Value>::erase (const Key& key) -> bool {
  if (auto it = cache_.find (key); it != cache_.end ()) {
    list_.erase (it->second);
    cache_.erase (it);
    return true;
  }
  return false;
}

template <typename Key, typename Value>
template <typename U, typename I>
auto lru_cache<Key, Value>::put (U&& key, I&& value) -> void {
  if (capacity_ == 0) {
    return;
  }

  if (auto it = cache_.find (key); it != cache_.end ()) {
    it->second->second = {std::forward<I> (value)};
    list_.splice (list_.begin (), list_, it->second);
    return;
  }

  if (cache_.size () >= capacity_) {
    // cache full evict oldest element
    auto old_entry_it = std::prev (list_.end ());
    cache_.erase ((*old_entry_it).first);
    list_.pop_back ();
  }

  list_.emplace_front (key, std::forward<I> (value));
  try {
    cache_.emplace (list_.front ().first, list_.begin ());
  } catch (...) {
    // roll back so list_ and cache_ stay in sync if map insertion fails
    list_.pop_front ();
    throw;
  }
}
}  // namespace utils
