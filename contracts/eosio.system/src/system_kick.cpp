#include <eosio.system/eosio.system.hpp>
#include <eosio/producer_schedule.hpp>

#define MAX_BLOCK_PER_CYCLE 12

namespace eosiosystem {
  using namespace eosio;

  bool system_contract::crossed_missed_blocks_threshold(uint32_t amountBlocksMissed, uint32_t schedule_size) {
    if (schedule_size <= 1) return false;

    auto timeframe = (_grotation.next_rotation_time.to_time_point() - _grotation.last_rotation_time.to_time_point()).to_seconds();
    // Total blocks that can be produced in a cycle
    auto maxBlocksPerCycle = (schedule_size - 1) * MAX_BLOCK_PER_CYCLE;
    // total block that can be produced in the current timeframe
    auto totalBlocksPerTimeframe = (maxBlocksPerCycle * timeframe) / (maxBlocksPerCycle / 2);
    // max blocks that can be produced by a single producer in a timeframe
    auto maxBlocksPerProducer = (totalBlocksPerTimeframe * MAX_BLOCK_PER_CYCLE) / maxBlocksPerCycle;
    // 15% is the max allowed missed blocks per single producer
    auto thresholdMissedBlocks = maxBlocksPerProducer * 0.15;

    return amountBlocksMissed > thresholdMissedBlocks;
  }

  void system_contract::reset_schedule_metrics(name producer = name(0)) {
    for (auto &pm : _gschedule_metrics.producers_metric) {
      if (producer != name(0) && pm.bp_name == producer) pm.missed_blocks_per_cycle = MAX_BLOCK_PER_CYCLE - 1;
      else pm.missed_blocks_per_cycle = MAX_BLOCK_PER_CYCLE;
    }
  }

  void system_contract::update_producer_missed_blocks(name producer) {
    for (auto &pm : _gschedule_metrics.producers_metric) {
      if (pm.bp_name == producer && pm.missed_blocks_per_cycle > 0) {
        pm.missed_blocks_per_cycle--;
        break;
      }
    }
  }

  bool system_contract::is_new_schedule_activated(name active_schedule[], uint32_t size) {
    std::vector<name> new_schedule;
    for (auto &p : _gschedule_metrics.producers_metric) new_schedule.emplace_back(p.bp_name);

    std::sort(new_schedule.begin(), new_schedule.end());
    std::sort(active_schedule, active_schedule + size);

    for (size_t i = 0; i < size; i++){
      if (active_schedule[i] != new_schedule[i]) return false;
    }

    return true;
  }

  bool system_contract::is_new_schedule_activated(std::vector<name>& active_schedule) {
    std::vector<name> new_schedule;
    for (auto &p : _gschedule_metrics.producers_metric) new_schedule.emplace_back(p.bp_name);

    std::sort(new_schedule.begin(), new_schedule.end());
    std::sort(active_schedule.begin(), active_schedule.end());

    for (size_t i = 0; i < active_schedule.size(); i++) {
      if (active_schedule[i] != new_schedule[i]) return false;
    }

    return true;
  }

  bool system_contract::check_missed_blocks(block_timestamp timestamp, name producer) {
    if (producer == "eosio"_n) {
      _gschedule_metrics.block_counter_correction++;
      _gschedule_metrics.last_onblock_caller = producer;
      return false;
    }

    auto producers_schedule = get_active_producers();
    bool is_activated = _gstate.last_producer_schedule_size == producers_schedule.size() && is_new_schedule_activated(producers_schedule);

    if (!is_activated) {
      if (_gschedule_metrics.last_onblock_caller != producer) _gschedule_metrics.block_counter_correction = 1;
      else _gschedule_metrics.block_counter_correction++;

      _gschedule_metrics.last_onblock_caller = producer;
      return false;
    } else if (_gschedule_metrics.block_counter_correction > 0) {
      if (_gschedule_metrics.last_onblock_caller == "eosio"_n) {
        for (auto &pm : _gschedule_metrics.producers_metric) {
          if (pm.bp_name == producer) {
            pm.missed_blocks_per_cycle -= uint32_t(_gschedule_metrics.block_counter_correction);
            break;
          }
        }
      } else {
          reset_schedule_metrics();
          _gschedule_metrics.block_counter_correction = -3;
      }
      _gschedule_metrics.last_onblock_caller = producer;
    }

    if (_gschedule_metrics.block_counter_correction < 0) {
      if (_gschedule_metrics.last_onblock_caller != producer && _gschedule_metrics.block_counter_correction < 0) {
        _gschedule_metrics.block_counter_correction++;
      }

      _gschedule_metrics.last_onblock_caller = producer;
      if (_gschedule_metrics.block_counter_correction < 0) {
        return false;
      }
    }

    auto pitr = _producers.find(producer.value);
    if (pitr != _producers.end() && !pitr->is_active) {
      reset_schedule_metrics();
      update_elected_producers(timestamp);
      return false;
    }

    if (_gschedule_metrics.last_onblock_caller != producer) {
      for (auto &pm : _gschedule_metrics.producers_metric) {
        if (pm.bp_name == producer) {                       
          if (pm.missed_blocks_per_cycle != MAX_BLOCK_PER_CYCLE) {        
            _gschedule_metrics.last_onblock_caller = producer;
            return true;
          }
          break;
        }
      }
    }
    
    update_producer_missed_blocks(producer);
    _gschedule_metrics.last_onblock_caller = producer;

    return false;
  }
}
