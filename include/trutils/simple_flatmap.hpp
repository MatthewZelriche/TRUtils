#pragma once

#include <functional>
#include <unordered_map>
#include <utility>
#include <vector>

namespace tr {

// Note/Todo: Mutating keys is undefined behavior. In the future we could
// create a custom iterator that returns const Key references instead.
template<typename Key, typename T, typename Hash = std::hash<Key>,
         typename KeyEqual = std::equal_to<Key>>
class simple_flatmap {
  public:
   using key_type = Key;
   using mapped_type = T;
   using value_type = std::pair<Key, T>;
   using size_type = std::size_t;
   using hasher = Hash;
   using key_equal = KeyEqual;
   using map_type = std::unordered_map<Key, size_type, Hash, KeyEqual>;
   using iterator = std::vector<value_type>::iterator;
   using const_iterator = std::vector<value_type>::const_iterator;

   simple_flatmap() = default;
   ~simple_flatmap() = default;
   simple_flatmap(const simple_flatmap &) = default;
   simple_flatmap(simple_flatmap &&) noexcept = default;
   simple_flatmap &operator=(const simple_flatmap &) = default;
   simple_flatmap &operator=(simple_flatmap &&) noexcept = default;

   bool empty() const { return mValues.empty(); }

   size_type size() const { return mValues.size(); }

   void clear() {
      mValues.clear();
      mIndex.clear();
   }

   bool contains(const Key &key) const { return mIndex.contains(key); }

   mapped_type &at(const Key &key) { return mValues[mIndex.at(key)].second; }

   const mapped_type &at(const Key &key) const { return mValues[mIndex.at(key)].second; }

   mapped_type &operator[](const Key &key) {
      auto [it, inserted] = mIndex.try_emplace(key, mValues.size());
      if (inserted) { mValues.emplace_back(key, T {}); }
      return mValues[it->second].second;
   }

   std::pair<bool, mapped_type &> insert(const Key &key, const mapped_type &value) {
      auto [it, inserted] = mIndex.try_emplace(key, mValues.size());
      if (!inserted) { return {false, mValues[it->second].second}; }

      mValues.push_back({key, value});
      return {true, mValues.back().second};
   }

   std::pair<bool, mapped_type &> insert(const Key &key, mapped_type &&value) {
      auto [it, inserted] = mIndex.try_emplace(key, mValues.size());
      if (!inserted) { return {false, mValues[it->second].second}; }

      mValues.push_back({key, std::move(value)});
      return {true, mValues.back().second};
   }

   bool erase(const Key &key) {
      if (!mIndex.contains(key)) { return false; }

      size_type idx = mIndex[key];
      size_type last = mValues.size() - 1;
      if (idx != last) {
         std::swap(mValues[idx], mValues[last]);
         mIndex[mValues[idx].first] = idx;
      }

      mIndex.erase(key);
      mValues.pop_back();
      return true;
   }

   void reserve(size_type count) {
      mValues.reserve(count);
      mIndex.reserve(count);
   }

   iterator begin() { return mValues.begin(); }
   iterator end() { return mValues.end(); }
   const_iterator begin() const { return mValues.begin(); }
   const_iterator end() const { return mValues.end(); }

  private:
   std::vector<value_type> mValues;
   map_type mIndex;
};

} // namespace tr
