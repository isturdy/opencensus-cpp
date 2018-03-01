// Copyright 2018, OpenCensus Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "opencensus/stats/internal/delta_producer.h"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <vector>

#include "absl/synchronization/mutex.h"
#include "absl/time/clock.h"
#include "absl/time/time.h"
#include "opencensus/stats/bucket_boundaries.h"
#include "opencensus/stats/internal/measure_data.h"
#include "opencensus/stats/internal/measure_registry_impl.h"
#include "opencensus/stats/internal/stats_manager.h"

namespace opencensus {
namespace stats {

void Delta::Record(std::initializer_list<Measurement> measurements,
                   TagSet tags) {
  auto it = delta_.find(tags);
  if (it == delta_.end()) {
    it = delta_.emplace_hint(it, std::move(tags),
                             absl::make_unique<MeasureData[]>(num_measures_));
  }
  for (const auto& measurement : measurements) {
    const uint64_t index = MeasureRegistryImpl::IdToIndex(measurement.id_);
    ABSL_ASSERT(index < registered_boundaries_.size());
    ABSL_ASSERT(index < num_measures_);
    const auto& boundaries = registered_boundaries_[index];
    switch (MeasureRegistryImpl::IdToType(measurement.id_)) {
      case MeasureDescriptor::Type::kDouble:
        it->second[index].Record(measurement.value_double_, boundaries);
        break;
      case MeasureDescriptor::Type::kInt64:
        it->second[index].Record(measurement.value_int_, boundaries);
        break;
    }
  }
}

void Delta::clear() {
  registered_boundaries_.clear();
  delta_.clear();
}

void Delta::SwapAndReset(int num_measures,
                         std::vector<absl::InlinedVector<BucketBoundaries, 1>>&
                             registered_boundaries,
                         Delta* other) {
  registered_boundaries_.swap(other->registered_boundaries_);
  delta_.swap(other->delta_);
  delta_.clear();
  num_measures_ = num_measures;
  registered_boundaries_ = registered_boundaries;
}

void Delta::Consume() {
  absl::Time now = absl::Now();
  for (const auto& tagset_stats : delta_) {
    for (int i = 0; i < num_measures_; ++i) {
      const MeasureData& data = tagset_stats.second[i];
      if (data.count() == 0) {
        // Shortcut if no data has been recorded for this tagset/measure pair.
        continue;
      }
      StatsManager::Get()->Record(i, data, registered_boundaries_,
                                  tagset_stats.first, now);
    }
  }
}

DeltaProducer* DeltaProducer::Get() {
  static DeltaProducer* global_delta_producer = new DeltaProducer();
  return global_delta_producer;
}

void DeltaProducer::AddMeasure() {
  absl::MutexLock delta_lock(&delta_mu_);
  absl::MutexLock harvester_lock(&harvester_mu_);
  ++num_measures_;
  registered_boundaries_.push_back({});
  FlushInternal();
}

void DeltaProducer::AddBoundaries(uint64_t index,
                                  const BucketBoundaries& boundaries) {
  absl::MutexLock delta_lock(&delta_mu_);
  auto& measure_boundaries = registered_boundaries_[index];
  if (std::find(measure_boundaries.begin(), measure_boundaries.end(),
                boundaries) == measure_boundaries.end()) {
    absl::MutexLock harvester_lock(&harvester_mu_);
    measure_boundaries.push_back(boundaries);
    FlushInternal();
  }
}

void DeltaProducer::Record(std::initializer_list<Measurement> measurements,
                           TagSet tags) {
  absl::MutexLock l(&delta_mu_);
  active_delta_.Record(measurements, std::move(tags));
}

void DeltaProducer::Flush() {
  absl::MutexLock delta_lock(&delta_mu_);
  absl::MutexLock harvester_lock(&harvester_mu_);
  FlushInternal();
}

DeltaProducer::DeltaProducer()
    : harvester_thread_(&DeltaProducer::RunHarvesterLoop, this) {}

void DeltaProducer::FlushInternal() {
  active_delta_.SwapAndReset(num_measures_, registered_boundaries_,
                             &last_delta_);
  last_delta_.Consume();
}

void DeltaProducer::RunHarvesterLoop() {
  absl::Time next_harvest_time = absl::Now() + harvest_interval_;
  while (true) {
    absl::SleepFor(next_harvest_time - absl::Now());
    next_harvest_time = absl::Now() + harvest_interval_;
    FlushInternal();
  }
}

}  // namespace stats
}  // namespace opencensus
