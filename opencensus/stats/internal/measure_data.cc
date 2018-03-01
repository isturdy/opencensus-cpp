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

#include "opencensus/stats/internal/measure_data.h"

#include <algorithm>
#include <cstdint>
#include <vector>

#include "absl/base/macros.h"
#include "absl/container/inlined_vector.h"
#include "opencensus/stats/bucket_boundaries.h"
#include "opencensus/stats/distribution.h"

namespace opencensus {
namespace stats {

void MeasureData::Record(
    double value, const absl::InlinedVector<BucketBoundaries, 1>& boundaries) {
  // Update using the method of provisional means.
  ++count_;
  ABSL_ASSERT(count_ > 0 && "Histogram count overflow.");
  const double new_mean = mean_ + (value - mean_) / count_;
  sum_of_squared_deviation_ =
      sum_of_squared_deviation_ + (value - mean_) * (value - new_mean);
  mean_ = new_mean;

  min_ = std::min(value, min_);
  max_ = std::max(value, max_);

  if (!boundaries.empty()) {
    if (histograms_.empty()) {
      histograms_.reserve(boundaries.size());
      for (const auto& bucketer : boundaries) {
        histograms_.emplace_back(bucketer.num_buckets());
      }
    }
    for (int i = 0; i < histograms_.size(); ++i) {
      ++histograms_[i][boundaries[i].BucketForValue(value)];
    }
  }
}

}  // namespace stats
}  // namespace opencensus
