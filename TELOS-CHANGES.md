## list of telos specific changes

compiled using this [Dockerfile](https://github.com/guilledk/telos-cdt-docker/blob/master/Dockerfile.cdt-3.0.0-rc2-antelope-3.1.0):

    # from repo root directory
    mkdir build

    # build from source
    docker run -it --rm \
        --mount type=bind,source="$(pwd)",target=/root/target \
        cdt:antelope \
        bash -c 'cd build && cmake .. -DBUILD_TESTS=yes && make'

    # run tests
    docker run -it --rm --mount \
        type=bind,source="$(pwd)",target=/root/target \
        cdt:antelope \
        bash -c 'cd build/tests && ./unit_test'

### `contracts/eosio.system/include`

#### `eosio.system.hpp`

    - include cmath, needed for pow
    - add custom bp rotations and payment system
    - decrease max ram size
    - add bp kick machinery

#### `native.hpp`

    - producer metric struct

#### `telos.hpp` (NEW)

    - telos specific constants

### `contracts/eosio.system/src`

#### `delegate_bandwidth.cpp`

    - disable b1 vesting validation
    - notify telos.decide on stake changes
    - change blockchain activation condition, from 15% staked, to 1,000,000 blocks produced

#### `eosio.system.cpp`

    - init telos specific tables
    - drop usage of `_gstate2` for `_gstate`
    - disable `setinflation`
    - add custom bp pay system action implementations

#### `limith_auth_changes.cpp`

    - disable `limithauthchg`

#### `powerup.cpp`

    - disable `cfgpowerup`, `powerupexec` & `powerup`

#### `producer_pay.cpp`

    - override `onblock` handler, custom bp rotation & pay system

#### `rex.cpp`

    - remove requirement to vote 21 BPs or select a proxy to stake to REX
    - `update_rex_account` null delta stake
    - disable rex pools

#### `system_kick.cpp` (NEW)

    - bp kick system helpers

#### `system_rotation.cpp` (NEW)

    - bp rotation system helpers

#### `voting.cpp`

    - add unreg reason
    - custom voting engine

### `tests/`

#### `eosio.limiauth_tests.cpp` (OFF)

#### `eosio.msig_tests.cpp`
    - disable `update_system_contract` test cases

#### `eosio.powerup_tests.cpp` (OFF)

#### `eosio.system_blockinfo_tests.cpp`

    - increase telos spent on resources for users due to differences with ram prices on stock antelope

#### `eosio.system_tester.hpp`

    - use legacy producer key
    - include telos constants
    - add custom telos system accounts to init list
    - change `get_rex_return_pool` logic
    - add `stake2votes` style helpers
    - add bp rotation and payment helpers
    - change `active_and_vote_producers` logic

#### `eosio.system_tests.cpp`

    - add custom telos tests
    - change chain activation function on all stock tests
    - delete tests cases:
        - `producer_wtmsig`
        - `producer_wtmsig_transition`
        - `proxy_*`
        - `change_inflation`
        - `extreme_inflation`
        - `votepay_*`
        - `namebid_pending_winner`
        - `wasmcfg`
        - `ram_inflation`
        - `rex_rounding_issue`
        - `b1_vesting`
        - `rex_return`

    - replace tests with custom ones:
        - `producer_pay`
        - `multi_producer_pay` 
        - `producers_upgrade_system_contract`

    - all other tests are either left the same or edited to match our custom changes

#### `eosio.wrap_tests.cpp`

    - import telos constants

#### `telos.hpp`

    - test helpers and constants

#### `test_symbol.hpp`

    - change test symbol from `TST` to `TLOS`
