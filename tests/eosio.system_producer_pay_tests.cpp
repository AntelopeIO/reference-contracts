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

BOOST_AUTO_TEST_SUITE(eosio_system_producer_pay_tests)

bool within_error(int64_t a, int64_t b, int64_t err) { return std::abs(a - b) <= err; };
bool within_one(int64_t a, int64_t b) { return within_error(a, b, 1); }

BOOST_FIXTURE_TEST_CASE(producer_pay, eosio_system_tester, * boost::unit_test::tolerance(1e-10)) try {

   const double continuous_rate = std::log1p(double(0.05));
   const double usecs_per_year  = 52 * 7 * 24 * 3600 * 1000000ll;
   const double secs_per_year   = 52 * 7 * 24 * 3600;


   const asset large_asset = core_sym::from_string("80.0000");
   create_account_with_resources( "defproducera"_n, config::system_account_name, core_sym::from_string("1.0000"), false, large_asset, large_asset );
   create_account_with_resources( "defproducerb"_n, config::system_account_name, core_sym::from_string("1.0000"), false, large_asset, large_asset );
   create_account_with_resources( "defproducerc"_n, config::system_account_name, core_sym::from_string("1.0000"), false, large_asset, large_asset );

   create_account_with_resources( "producvotera"_n, config::system_account_name, core_sym::from_string("1.0000"), false, large_asset, large_asset );
   create_account_with_resources( "producvoterb"_n, config::system_account_name, core_sym::from_string("1.0000"), false, large_asset, large_asset );

   BOOST_REQUIRE_EQUAL(success(), regproducer("defproducera"_n));
   produce_block(fc::hours(24));
   auto prod = get_producer_info( "defproducera"_n );
   BOOST_REQUIRE_EQUAL("defproducera", prod["owner"].as_string());
   BOOST_REQUIRE_EQUAL(0, prod["total_votes"].as_double());

   transfer( config::system_account_name, "producvotera", core_sym::from_string("400000000.0000"), config::system_account_name);
   BOOST_REQUIRE_EQUAL(success(), stake("producvotera", core_sym::from_string("100000000.0000"), core_sym::from_string("100000000.0000")));
   BOOST_REQUIRE_EQUAL(success(), vote( "producvotera"_n, { "defproducera"_n }));
   // defproducera is the only active producer
   // produce enough blocks so new schedule kicks in and defproducera produces some blocks
   {
      produce_blocks(50);

      const auto     initial_global_state      = get_global_state();
      const uint64_t initial_claim_time        = microseconds_since_epoch_of_iso_string( initial_global_state["last_pervote_bucket_fill"] );
      const int64_t  initial_pervote_bucket    = initial_global_state["pervote_bucket"].as<int64_t>();
      const int64_t  initial_perblock_bucket   = initial_global_state["perblock_bucket"].as<int64_t>();
      const int64_t  initial_savings           = get_balance("eosio.saving"_n).get_amount();
      const uint32_t initial_tot_unpaid_blocks = initial_global_state["total_unpaid_blocks"].as<uint32_t>();

      prod = get_producer_info("defproducera");
      const uint32_t unpaid_blocks = prod["unpaid_blocks"].as<uint32_t>();
      BOOST_REQUIRE(1 < unpaid_blocks);

      BOOST_REQUIRE_EQUAL(initial_tot_unpaid_blocks, unpaid_blocks);

      const asset initial_supply  = get_token_supply();
      const asset initial_balance = get_balance("defproducera"_n);

      BOOST_REQUIRE_EQUAL(success(), push_action("defproducera"_n, "claimrewards"_n, mvo()("owner", "defproducera")));

      const auto     global_state      = get_global_state();
      const uint64_t claim_time        = microseconds_since_epoch_of_iso_string( global_state["last_pervote_bucket_fill"] );
      const int64_t  pervote_bucket    = global_state["pervote_bucket"].as<int64_t>();
      const int64_t  perblock_bucket   = global_state["perblock_bucket"].as<int64_t>();
      const int64_t  savings           = get_balance("eosio.saving"_n).get_amount();
      const uint32_t tot_unpaid_blocks = global_state["total_unpaid_blocks"].as<uint32_t>();

      prod = get_producer_info("defproducera");
      BOOST_REQUIRE_EQUAL(1, prod["unpaid_blocks"].as<uint32_t>());
      BOOST_REQUIRE_EQUAL(1, tot_unpaid_blocks);
      const asset supply  = get_token_supply();
      const asset balance = get_balance("defproducera"_n);

      BOOST_REQUIRE_EQUAL(claim_time, microseconds_since_epoch_of_iso_string( prod["last_claim_time"] ));

      auto usecs_between_fills = claim_time - initial_claim_time;
      int32_t secs_between_fills = usecs_between_fills/1000000;

      BOOST_REQUIRE_EQUAL(0, initial_savings);
      BOOST_REQUIRE_EQUAL(0, initial_perblock_bucket);
      BOOST_REQUIRE_EQUAL(0, initial_pervote_bucket);

      BOOST_REQUIRE_EQUAL(int64_t( ( initial_supply.get_amount() * double(secs_between_fills) * continuous_rate ) / secs_per_year ),
                          supply.get_amount() - initial_supply.get_amount());
      BOOST_REQUIRE(within_one(int64_t( ( initial_supply.get_amount() * double(secs_between_fills) * (4.   * continuous_rate/ 5.) / secs_per_year ) ),
                               savings - initial_savings));
      BOOST_REQUIRE(within_one(int64_t( ( initial_supply.get_amount() * double(secs_between_fills) * (0.25 * continuous_rate/ 5.) / secs_per_year ) ),
                               balance.get_amount() - initial_balance.get_amount()));

      int64_t from_perblock_bucket = int64_t( initial_supply.get_amount() * double(secs_between_fills) * (0.25 * continuous_rate/ 5.) / secs_per_year ) ;
      int64_t from_pervote_bucket  = int64_t( initial_supply.get_amount() * double(secs_between_fills) * (0.75 * continuous_rate/ 5.) / secs_per_year ) ;


      if (from_pervote_bucket >= 100 * 10000) {
         BOOST_REQUIRE_EQUAL(from_perblock_bucket + from_pervote_bucket, balance.get_amount() - initial_balance.get_amount());
         BOOST_REQUIRE_EQUAL(0, pervote_bucket);
      } else {
         BOOST_REQUIRE_EQUAL(from_perblock_bucket, balance.get_amount() - initial_balance.get_amount());
         BOOST_REQUIRE_EQUAL(from_pervote_bucket, pervote_bucket);
      }
   }

   {
      BOOST_REQUIRE_EQUAL(wasm_assert_msg("already claimed rewards within past day"),
                          push_action("defproducera"_n, "claimrewards"_n, mvo()("owner", "defproducera")));
   }

   // defproducera waits for 23 hours and 55 minutes, can't claim rewards yet
   {
      produce_block(fc::seconds(23 * 3600 + 55 * 60));
      BOOST_REQUIRE_EQUAL(wasm_assert_msg("already claimed rewards within past day"),
                          push_action("defproducera"_n, "claimrewards"_n, mvo()("owner", "defproducera")));
   }

   // wait 5 more minutes, defproducera can now claim rewards again
   {
      produce_block(fc::seconds(5 * 60));

      const auto     initial_global_state      = get_global_state();
      const uint64_t initial_claim_time        = microseconds_since_epoch_of_iso_string( initial_global_state["last_pervote_bucket_fill"] );
      const int64_t  initial_pervote_bucket    = initial_global_state["pervote_bucket"].as<int64_t>();
      const int64_t  initial_perblock_bucket   = initial_global_state["perblock_bucket"].as<int64_t>();
      const int64_t  initial_savings           = get_balance("eosio.saving"_n).get_amount();
      const uint32_t initial_tot_unpaid_blocks = initial_global_state["total_unpaid_blocks"].as<uint32_t>();
      const double   initial_tot_vote_weight   = initial_global_state["total_producer_vote_weight"].as<double>();

      prod = get_producer_info("defproducera");
      const uint32_t unpaid_blocks = prod["unpaid_blocks"].as<uint32_t>();
      BOOST_REQUIRE(1 < unpaid_blocks);
      BOOST_REQUIRE_EQUAL(initial_tot_unpaid_blocks, unpaid_blocks);
      BOOST_REQUIRE(0 < prod["total_votes"].as<double>());
      BOOST_TEST(initial_tot_vote_weight, prod["total_votes"].as<double>());
      BOOST_REQUIRE(0 < microseconds_since_epoch_of_iso_string( prod["last_claim_time"] ));

      BOOST_REQUIRE_EQUAL(initial_tot_unpaid_blocks, unpaid_blocks);

      const asset initial_supply  = get_token_supply();
      const asset initial_balance = get_balance("defproducera"_n);

      BOOST_REQUIRE_EQUAL(success(), push_action("defproducera"_n, "claimrewards"_n, mvo()("owner", "defproducera")));

      const auto global_state          = get_global_state();
      const uint64_t claim_time        = microseconds_since_epoch_of_iso_string( global_state["last_pervote_bucket_fill"] );
      const int64_t  pervote_bucket    = global_state["pervote_bucket"].as<int64_t>();
      const int64_t  perblock_bucket   = global_state["perblock_bucket"].as<int64_t>();
      const int64_t  savings           = get_balance("eosio.saving"_n).get_amount();
      const uint32_t tot_unpaid_blocks = global_state["total_unpaid_blocks"].as<uint32_t>();

      prod = get_producer_info("defproducera");
      BOOST_REQUIRE_EQUAL(1, prod["unpaid_blocks"].as<uint32_t>());
      BOOST_REQUIRE_EQUAL(1, tot_unpaid_blocks);
      const asset supply  = get_token_supply();
      const asset balance = get_balance("defproducera"_n);

      BOOST_REQUIRE_EQUAL(claim_time, microseconds_since_epoch_of_iso_string( prod["last_claim_time"] ));
      auto usecs_between_fills = claim_time - initial_claim_time;

      BOOST_REQUIRE_EQUAL( int64_t( initial_supply.get_amount() * double(usecs_between_fills) * continuous_rate / usecs_per_year ),
                           supply.get_amount() - initial_supply.get_amount() );
      BOOST_REQUIRE_EQUAL( (supply.get_amount() - initial_supply.get_amount()) - (supply.get_amount() - initial_supply.get_amount()) / 5,
                           savings - initial_savings );

      int64_t to_producer        = int64_t( initial_supply.get_amount() * double(usecs_between_fills) * continuous_rate / usecs_per_year ) / 5;
      int64_t to_perblock_bucket = to_producer / 4;
      int64_t to_pervote_bucket  = to_producer - to_perblock_bucket;

      if (to_pervote_bucket + initial_pervote_bucket >= 100 * 10000) {
         BOOST_REQUIRE_EQUAL(to_perblock_bucket + to_pervote_bucket + initial_pervote_bucket, balance.get_amount() - initial_balance.get_amount());
         BOOST_REQUIRE_EQUAL(0, pervote_bucket);
      } else {
         BOOST_REQUIRE_EQUAL(to_perblock_bucket, balance.get_amount() - initial_balance.get_amount());
         BOOST_REQUIRE_EQUAL(to_pervote_bucket + initial_pervote_bucket, pervote_bucket);
      }
   }

   // defproducerb tries to claim rewards but he's not on the list
   {
      BOOST_REQUIRE_EQUAL(wasm_assert_msg("producer not registered"),
                          push_action("defproducerb"_n, "claimrewards"_n, mvo()("owner", "defproducerb")));
   }


   // test claimrewards when max supply is reached
   {
      produce_block(fc::hours(24));

      const asset before_supply = get_token_supply();
      const asset before_system_balance = get_balance(config::system_account_name);
      const asset before_producer_balance = get_balance("defproducera"_n);

      setmaxsupply( before_supply );
      BOOST_REQUIRE_EQUAL(success(), push_action("defproducera"_n, "claimrewards"_n, mvo()("owner", "defproducera")));

      const asset after_supply = get_token_supply();
      const asset after_system_balance = get_balance(config::system_account_name);
      const asset after_producer_balance = get_balance("defproducera"_n);

      BOOST_REQUIRE_EQUAL(after_supply.get_amount() - before_supply.get_amount(), 0);
      BOOST_REQUIRE_EQUAL(after_system_balance.get_amount() - before_system_balance.get_amount(), -1340576600);
      BOOST_REQUIRE_EQUAL(after_producer_balance.get_amount() - before_producer_balance.get_amount(), 268115320);
   }

   // test stability over a year
   {
      regproducer("defproducerb"_n);
      regproducer("defproducerc"_n);
      produce_block(fc::hours(24));
      const asset   initial_supply  = get_token_supply();
      const int64_t initial_savings = get_balance("eosio.saving"_n).get_amount();
      for (uint32_t i = 0; i < 7 * 52; ++i) {
         produce_block(fc::seconds(8 * 3600));
         BOOST_REQUIRE_EQUAL(success(), push_action("defproducerc"_n, "claimrewards"_n, mvo()("owner", "defproducerc")));
         produce_block(fc::seconds(8 * 3600));
         BOOST_REQUIRE_EQUAL(success(), push_action("defproducerb"_n, "claimrewards"_n, mvo()("owner", "defproducerb")));
         produce_block(fc::seconds(8 * 3600));
         BOOST_REQUIRE_EQUAL(success(), push_action("defproducera"_n, "claimrewards"_n, mvo()("owner", "defproducera")));
      }
      const asset   supply  = get_token_supply();
      const int64_t savings = get_balance("eosio.saving"_n).get_amount();
      // Amount issued per year is very close to the 5% inflation target. Small difference (500 tokens out of 50'000'000 issued)
      // is due to compounding every 8 hours in this test as opposed to theoretical continuous compounding
      BOOST_REQUIRE(500 * 10000 > int64_t(double(initial_supply.get_amount()) * double(0.05)) - (supply.get_amount() - initial_supply.get_amount()));
      BOOST_REQUIRE(500 * 10000 > int64_t(double(initial_supply.get_amount()) * double(0.04)) - (savings - initial_savings));
   }

} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_SUITE_END()
