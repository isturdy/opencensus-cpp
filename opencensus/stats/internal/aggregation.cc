// Copyright 2017, OpenCensus Authors
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

#include "opencensus/stats/aggregation.h"

#include "absl/strings/str_cat.h"

namespace opencensus {
namespace stats {

std::string Aggregation::DebugString() const {
  switch (type_) {
    case Type::kCount: {
      return "Count";
      break;
    }
    case Type::kSum: {
      return "Sum";
      break;
    }
    case Type::kDistribution: {
      return absl::StrCat("Distribution with ",
                          bucket_boundaries_.DebugString());
    }
  }
  assert(false);
}

}  // namespace stats
}  // namespace opencensus
