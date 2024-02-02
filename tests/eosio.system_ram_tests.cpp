#include <boost/test/unit_test.hpp>
#include <eosio/chain/contract_table_objects.hpp>
#include <eosio/chain/global_property_object.hpp>
#include <eosio/chain/resource_limits.hpp>
#include <eosio/chain/wast_to_wasm.hpp>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <fc/log/logger.hpp>
#include <eosio/chain/exceptions.hpp>

#include "eosio.system_tester.hpp"

using namespace eosio_system;

BOOST_AUTO_TEST_SUITE(eosio_system_ram_tests);

// ramtransfer
BOOST_FIXTURE_TEST_CASE( ram_transfer, eosio_system_tester ) try {
   const std::vector<account_name> accounts = { "alice"_n, "bob"_n };
   create_accounts_with_resources( accounts );
   const account_name alice = accounts[0];
   const account_name bob = accounts[1];

   transfer( config::system_account_name, alice, core_sym::from_string("100.0000"), config::system_account_name );
   transfer( config::system_account_name, bob, core_sym::from_string("100.0000"), config::system_account_name );
   BOOST_REQUIRE_EQUAL( success(), buyrambytes( alice, alice, 10000 ) );
   BOOST_REQUIRE_EQUAL( success(), buyrambytes( bob, bob, 10000 ) );

   const uint64_t alice_before = get_total_stake( alice )["ram_bytes"].as_uint64();
   const uint64_t bob_before = get_total_stake( bob )["ram_bytes"].as_uint64();

   ramtransfer( alice, bob, 1000, "" );

   const uint64_t alice_after = get_total_stake( alice )["ram_bytes"].as_uint64();
   const uint64_t bob_after = get_total_stake( bob )["ram_bytes"].as_uint64();

   BOOST_REQUIRE_EQUAL( alice_before - 1000, alice_after );
   BOOST_REQUIRE_EQUAL( bob_before + 1000, bob_after );

   const char* expected_return_data = R"=====(
{
   "from": "alice",
   "to": "bob",
   "bytes": 1,
   "from_ram_bytes": 16982,
   "to_ram_bytes": 18984
}
)=====";
   validate_ramtransfer_return(alice, bob, 1, "", "action_return_ramtransfer", expected_return_data );

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( buy_sell_ram_validate, eosio_system_tester ) try {
   const std::vector<account_name> accounts = { "alice"_n };
   create_accounts_with_resources( accounts );
   const account_name alice = accounts[0];

   transfer( config::system_account_name, alice, core_sym::from_string("100.0000"), config::system_account_name );

   const char* expected_buyrambytes_return_data = R"=====(
{
   "payer": "alice",
   "receiver": "alice",
   "quantity": "0.1462 TST",
   "bytes": 9991,
   "ram_bytes": 17983
}
)=====";
   validate_buyrambytes_return(alice, alice, 10000, "action_return_buyram", expected_buyrambytes_return_data );

const char* expected_sellram_return_data = R"=====(
{
   "account": "alice",
   "quantity": "0.1455 TST",
   "bytes": 10000,
   "ram_bytes": 7983
}
)=====";
   validate_sellram_return(alice, 10000, "action_return_sellram", expected_sellram_return_data );

} FC_LOG_AND_RETHROW()

// ramburn
BOOST_FIXTURE_TEST_CASE( ram_burn, eosio_system_tester ) try {
   const std::vector<account_name> accounts = { "alice"_n };
   create_accounts_with_resources( accounts );
   const account_name alice = accounts[0];
   const account_name null_account = "eosio.null"_n;

   transfer( config::system_account_name, alice, core_sym::from_string("100.0000"), config::system_account_name );
   BOOST_REQUIRE_EQUAL( success(), buyrambytes( alice, alice, 10000 ) );
   BOOST_REQUIRE_EQUAL( success(), buyrambytes( alice, null_account, 10000 ) );

   const uint64_t null_before_burn = get_total_stake( null_account )["ram_bytes"].as_uint64();
   const uint64_t alice_before_burn = get_total_stake( alice )["ram_bytes"].as_uint64();

   // burn action
   BOOST_REQUIRE_EQUAL( success(), ramburn( alice, 3000, "burn RAM memo" ) );
   const uint64_t alice_after_burn = get_total_stake( alice )["ram_bytes"].as_uint64();
   const uint64_t null_after_burn = get_total_stake( null_account )["ram_bytes"].as_uint64();
   BOOST_REQUIRE_EQUAL( alice_before_burn - 3000, alice_after_burn );
   BOOST_REQUIRE_EQUAL( null_before_burn + 3000, null_after_burn );

} FC_LOG_AND_RETHROW()


// buyramself
BOOST_FIXTURE_TEST_CASE( buy_ram_self, eosio_system_tester ) try {
   const std::vector<account_name> accounts = { "alice"_n };
   create_accounts_with_resources( accounts );
   const account_name alice = accounts[0];

   transfer( config::system_account_name, alice, core_sym::from_string("100.0000"), config::system_account_name );
   const uint64_t alice_before = get_total_stake( alice )["ram_bytes"].as_uint64();
   BOOST_REQUIRE_EQUAL( success(), buyramself( alice, core_sym::from_string("1.0000")) );
   const uint64_t alice_after = get_total_stake( alice )["ram_bytes"].as_uint64();
   BOOST_REQUIRE_EQUAL( alice_before + 68375, alice_after );

} FC_LOG_AND_RETHROW()


BOOST_AUTO_TEST_SUITE_END()
