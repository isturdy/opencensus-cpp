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

#ifndef OPENCENSUS_STATS_INTERNAL_DELTA_PRODUCER_H_
#define OPENCENSUS_STATS_INTERNAL_DELTA_PRODUCER_H_

#include <cstdint>
#include <memory>
#include <thread>
#include <unordered_map>
#include <vector>

#include "absl/container/inlined_vector.h"
#include "absl/synchronization/mutex.h"
#include "absl/time/time.h"
#include "opencensus/stats/bucket_boundaries.h"
#include "opencensus/stats/distribution.h"
#include "opencensus/stats/internal/measure_data.h"
#include "opencensus/stats/measure.h"
#include "opencensus/stats/tag_set.h"

namespace opencensus {
namespace stats {

// Delta is thread-compatible.
class Delta final {
 public:
  void Record(std::initializer_list<Measurement> measurements, TagSet tags);

  void clear();
  void SwapAndReset(int num_measures,
                    std::vector<absl::InlinedVector<BucketBoundaries, 1>>&
                        registered_boundaries,
                    Delta* other);

  void Consume();

 private:
  // Copies of the corresponding fields in the DeltaProducer as of when the
  // delta was started.
  int num_measures_;
  std::vector<absl::InlinedVector<BucketBoundaries, 1>> registered_boundaries_;

  // The actual data. Each MeasureData[] contains one element for each
  // registered measure.
  std::unordered_map<TagSet, std::unique_ptr<MeasureData[]>, TagSet::Hash>
      delta_;
};

// DeltaProducer is thread-safe.
class DeltaProducer final {
 public:
  // Returns a pointer to the singleton DeltaProducer.
  static DeltaProducer* Get();

  void AddMeasure();

  // Adds a new BucketBoundaries for the measure 'index' if it does not already
  // exist.
  void AddBoundaries(uint64_t index, const BucketBoundaries& boundaries);

  void Record(std::initializer_list<Measurement> measurements, TagSet tags)
      LOCKS_EXCLUDED(delta_mu_);

  // Flushes the active delta and blocks until it is harvested.
  void Flush() LOCKS_EXCLUDED(delta_mu_, harvester_mu_);

 private:
  DeltaProducer();

  // Triggers a flush of the active delta, but returns immediately without
  // waiting for the delta to be consumed.
  void FlushInternal() EXCLUSIVE_LOCKS_REQUIRED(delta_mu_, harvester_mu_);

  void RunHarvesterLoop();

  const absl::Duration harvest_interval_ = absl::Seconds(5);

  // Guards the active delta and its configuration. Anything that changes the
  // delta configuration (e.g. adding a measure or BucketBoundaries) must
  // acquire delta_mu_, update configuration, and call FlushInternal() before
  // releasing delta_mu_ to prevent a Record() from accessing the delta with
  // mismatched configuration.
  absl::Mutex delta_mu_;

  int num_measures_ GUARDED_BY(delta_mu_);
  // The BucketBoundaries of each registered view with Distribution aggregation,
  // by measure. Array indices in the outer array correspond to measure indices.
  //
  // Uses InlinedVector to avoid nested copies during the common case of having
  // one registered set of boundaries per measure.
  std::vector<absl::InlinedVector<BucketBoundaries, 1>> registered_boundaries_
      GUARDED_BY(delta_mu_);
  Delta active_delta_ GUARDED_BY(delta_mu_);

  // Guards the last_delta_; acquired by the main thread when triggering a
  // flush.
  absl::Mutex harvester_mu_ ACQUIRED_AFTER(delta_mu_);
  // TODO: consider making this a lockless queue to avoid blocking the main
  // thread when calling a flush during harvesting.
  Delta last_delta_ GUARDED_BY(harvester_mu_);
  std::thread harvester_thread_ GUARDED_BY(harvester_mu_);
};

}  // namespace stats
}  // namespace opencensus

#endif  // OPENCENSUS_STATS_INTERNAL_DELTA_PRODUCER_H_
