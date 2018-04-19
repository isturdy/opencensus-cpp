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

#include "opencensus/stats/tag_set.h"

#include <cstring>
#include <functional>
#include <initializer_list>
#include <string>
#include <utility>
#include <vector>

#include "absl/strings/string_view.h"
#include "opencensus/common/internal/hash_mix.h"
#include "opencensus/stats/tag_key.h"

namespace opencensus {
namespace stats {

TagSet::TagSet(
    std::initializer_list<std::pair<TagKey, absl::string_view>> tags) {
  tags_.reserve(tags.size());
  for (const auto& tag : tags) {
    tags_.emplace_back(tag.first, std::string(tag.second));
  }
  std::sort(tags_.begin(), tags_.end());
  Initialize();
}

TagSet::TagSet(std::vector<std::pair<TagKey, std::string>> tags)
    : tags_(std::move(tags)) {
  std::sort(tags_.begin(), tags_.end());
  Initialize();
}

void TagSet::SetTags(
    std::initializer_list<std::pair<TagKey, absl::string_view>> tags) {
  std::vector<std::pair<TagKey, absl::string_view>> sorted_tags(tags);
  std::sort(sorted_tags.begin(), sorted_tags.end());

  // Merge/insert
  int existing_tag_index = 0;
  int new_tag_index = 0;
  while (existing_tag_index < tags_.size() &&
         new_tag_index < sorted_tags.size()) {
    const TagKey old_key = tags_[existing_tag_index].first;
    const TagKey new_key = sorted_tags[new_tag_index].first;
    if (old_key < new_key) {
      ++existing_tag_index;
    } else if (old_key == new_key) {
      tags_[existing_tag_index].second =
          std::string(sorted_tags[new_tag_index].second);
      ++existing_tag_index;
      ++new_tag_index;
    } else {
      tags_.insert(
          tags_.begin() + existing_tag_index,
          std::pair<TagKey, std::string>(
              new_key, std::string(sorted_tags[new_tag_index].second)));
      ++existing_tag_index;
      ++new_tag_index;
    }
  }
  while (new_tag_index < sorted_tags.size()) {
    tags_.emplace_back(sorted_tags[new_tag_index].first,
                       std::string(sorted_tags[new_tag_index].second));
    ++new_tag_index;
  }
  Initialize();
}

void TagSet::Initialize() {
  std::hash<std::string> hasher;
  common::HashMix mixer;
  for (const auto& tag : tags_) {
    mixer.Mix(tag.first.hash());
    mixer.Mix(hasher(tag.second));
  }
  hash_ = mixer.get();
}

std::size_t TagSet::Hash::operator()(const TagSet& tag_set) const {
  return tag_set.hash_;
}

bool TagSet::operator==(const TagSet& other) const {
  return tags_ == other.tags_;
}

}  // namespace stats
}  // namespace opencensus
