#pragma once

#include <eosio/chain/name.hpp>

using namespace eosio::chain;

/**
 * name literal operator
 * 
 * @brief "foo"_n is a shortcut for name{eosio::string_to_name("foo")}
 */
inline constexpr name operator""_n(const char* s, std::size_t) {
   return {string_to_name(s)};
}
