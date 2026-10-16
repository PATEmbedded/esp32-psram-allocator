// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Pourya Afshintabar
//
// Standard containers whose storage lives in PSRAM.
//
//   psram::vector<T>            PSRAM only
//   psram::fallback::vector<T>  PSRAM first, internal RAM when PSRAM is full

#pragma once

#include <deque>
#include <forward_list>
#include <functional>
#include <list>
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "allocator.hpp"

namespace psram {

template <class T, class Policy = external_only>
using vector = std::vector<T, allocator<T, Policy>>;

template <class T, class Policy = external_only>
using deque = std::deque<T, allocator<T, Policy>>;

template <class T, class Policy = external_only>
using list = std::list<T, allocator<T, Policy>>;

template <class T, class Policy = external_only>
using forward_list = std::forward_list<T, allocator<T, Policy>>;

template <class CharT, class Policy = external_only, class Traits = std::char_traits<CharT>>
using basic_string = std::basic_string<CharT, Traits, allocator<CharT, Policy>>;

using string = basic_string<char>;

/// Hash for strings with any allocator. C++17 only provides std::hash for
/// strings that use std::allocator.
struct string_hash {
    template <class CharT, class Traits, class Alloc>
    std::size_t operator()(const std::basic_string<CharT, Traits, Alloc>& s) const noexcept {
        return std::hash<std::basic_string_view<CharT, Traits>>{}(s);
    }
};

namespace detail {

template <class Key>
struct default_hash {
    using type = std::hash<Key>;
};

template <class CharT, class Traits, class Policy>
struct default_hash<std::basic_string<CharT, Traits, allocator<CharT, Policy>>> {
    using type = string_hash;
};

}  // namespace detail

/// std::hash<Key>, or string_hash when Key is a psram string.
template <class Key>
using default_hash_t = typename detail::default_hash<Key>::type;

template <class Key, class T, class Compare = std::less<Key>, class Policy = external_only>
using map = std::map<Key, T, Compare, allocator<std::pair<const Key, T>, Policy>>;

template <class Key, class T, class Compare = std::less<Key>, class Policy = external_only>
using multimap = std::multimap<Key, T, Compare, allocator<std::pair<const Key, T>, Policy>>;

template <class Key, class Compare = std::less<Key>, class Policy = external_only>
using set = std::set<Key, Compare, allocator<Key, Policy>>;

template <class Key, class Compare = std::less<Key>, class Policy = external_only>
using multiset = std::multiset<Key, Compare, allocator<Key, Policy>>;

template <class Key, class T, class Hash = default_hash_t<Key>, class KeyEqual = std::equal_to<Key>,
          class Policy = external_only>
using unordered_map = std::unordered_map<Key, T, Hash, KeyEqual, allocator<std::pair<const Key, T>, Policy>>;

template <class Key, class Hash = default_hash_t<Key>, class KeyEqual = std::equal_to<Key>,
          class Policy = external_only>
using unordered_set = std::unordered_set<Key, Hash, KeyEqual, allocator<Key, Policy>>;

/// The same containers with the prefer_external policy.
namespace fallback {

template <class T>
using vector = psram::vector<T, prefer_external>;

template <class T>
using deque = psram::deque<T, prefer_external>;

template <class T>
using list = psram::list<T, prefer_external>;

template <class T>
using forward_list = psram::forward_list<T, prefer_external>;

using string = psram::basic_string<char, prefer_external>;

template <class Key, class T, class Compare = std::less<Key>>
using map = psram::map<Key, T, Compare, prefer_external>;

template <class Key, class T, class Compare = std::less<Key>>
using multimap = psram::multimap<Key, T, Compare, prefer_external>;

template <class Key, class Compare = std::less<Key>>
using set = psram::set<Key, Compare, prefer_external>;

template <class Key, class Compare = std::less<Key>>
using multiset = psram::multiset<Key, Compare, prefer_external>;

template <class Key, class T, class Hash = default_hash_t<Key>, class KeyEqual = std::equal_to<Key>>
using unordered_map = psram::unordered_map<Key, T, Hash, KeyEqual, prefer_external>;

template <class Key, class Hash = default_hash_t<Key>, class KeyEqual = std::equal_to<Key>>
using unordered_set = psram::unordered_set<Key, Hash, KeyEqual, prefer_external>;

}  // namespace fallback

}  // namespace psram
