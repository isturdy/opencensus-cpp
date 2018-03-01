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

#ifndef OPENCENSUS_STATS_INTERNAL_MEASURE_DATA_H_
#define OPENCENSUS_STATS_INTERNAL_MEASURE_DATA_H_

#include <cstdint>
#include <vector>

#include "absl/container/inlined_vector.h"
#include "opencensus/stats/bucket_boundaries.h"
#include "opencensus/stats/distribution.h"

namespace opencensus {
namespace stats {

// MeasureData tracks all aggregations for a single measure, including
// histograms for all views registered on that measure. The BucketBoundaries of
// those histograms are stored separately, and must be consistent across calls
// to all methods for the lifetime of the object.
//
// MeasureData is thread-compatible.
class MeasureData final {
 public:
  // Records 'value' using the provided boundaries. The first call initializes
  // the histograms; all subsequent calls must use the same boundaries.
  void Record(double value,
              const absl::InlinedVector<BucketBoundaries, 1>& boundaries);

  uint64_t count() const { return count_; }
  double sum() const { return count_ * mean_; }
  void AddToDistribution(
      const absl::InlinedVector<BucketBoundaries, 1>& boundaries,
      Distribution* distribution) const;

 private:
  uint64_t count_ = 0;
  double mean_ = 0;
  double sum_of_squared_deviation_ = 0;
  double min_ = std::numeric_limits<double>::infinity();
  double max_ = -std::numeric_limits<double>::infinity();
  // Avoid pointer chasing for the common case of 1 BucketBoundarie.
  absl::InlinedVector<std::vector<int64_t>, 1> histograms_;
};

}  // namespace stats
}  // namespace opencensus

#endif  // OPENCENSUS_STATS_INTERNAL_MEASURE_DATA_H_
