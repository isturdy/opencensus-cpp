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

#ifndef OPENCENSUS_STATS_TAG_SET_H_
#define OPENCENSUS_STATS_TAG_SET_H_

#include <initializer_list>
#include <string>
#include <utility>
#include <vector>

#include "absl/strings/string_view.h"

namespace opencensus {
namespace stats {

//
class TagSet final {
 public:
  // Not explicit so that Record({}, {{"k", "v"}}) works;
  TagSet(std::initializer_list<std::pair<absl::string_view, absl::string_view>>
             tags);
  TagSet(std::vector<std::pair<std::string, std::string>> tags);

  const std::vector<std::pair<std::string, std::string>>& tags() const {
    return tags_;
  }

  struct Hash {
    std::size_t operator()(const TagSet& tag_set) const;
  };

  bool operator==(const TagSet& other) const;

 private:
  void Initialize();

  std::size_t hash_;
  // TODO: add an option to store string_view to avoid copies on lookup.
  std::vector<std::pair<std::string, std::string>> tags_;
};

}  // namespace stats
}  // namespace opencensus

#endif  // OPENCENSUS_STATS_TAG_SET_H_
