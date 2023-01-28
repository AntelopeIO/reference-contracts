---
content_title: About System Contracts
---

The EOS blockchain is unique in that the features and characteristics of the blockchain built on it are flexible, that is, they can be changed, or modified completely to suit the EOS community needs. Core blockchain features such as consensus, fee schedules, account creation and modification, token economics, block producer registration, voting, multi-sig, are implemented inside smart contracts which are deployed on the EOS blockchain.

## System contracts defined in eos-system-contracts

The `eos-system-contracts` repository contains the system contracts encapsulating the base functionality for the EOS blockchain.

1. [eosio.bios](action-reference/eosio.bios)
2. [eosio.system](action-reference/eosio.system)
3. [eosio.msig](action-reference/eosio.msig)
4. [eosio.token](action-reference/eosio.token)
5. [eosio.wrap](action-reference/eosio.wrap)

## Key Concepts Implemented by eosio.system

1. [System](01_key-concepts/01_system.md)
2. [RAM](01_key-concepts/02_ram.md)
3. [CPU](01_key-concepts/03_cpu.md)
4. [NET](01_key-concepts/04_net.md)
5. [Resource Allocation](01_key-concepts/05_system_resource_allocation.md)
6. [Vote](01_key-concepts/06_vote.md)

## Build and deploy

To build and deploy the system contract follow the instruction from [Build and deploy](03_build-and-deploy.md) section.