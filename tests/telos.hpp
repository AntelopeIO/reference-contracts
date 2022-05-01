#pragma once

#include <eosio/chain/name.hpp>

using namespace eosio::chain;

#define MAX_PRODUCERS 42     // revised for TEDP 2 Phase 2, also set in producer_pay.cpp, change in both places
#define TOP_PRODUCERS 21
#define MAX_VOTE_PRODUCERS 30


/**
 * name literal operator
 * 
 * @brief "foo"_n is a shortcut for name{eosio::string_to_name("foo")}
 */
inline constexpr name operator""_n(const char* s, std::size_t) {
   return {string_to_name(s)};
}
