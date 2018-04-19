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

#include <cstdint>
#include <functional>
#include <string>
#include <utility>
#include <vector>

#include "absl/strings/str_cat.h"
#include "benchmark/benchmark.h"
#include "opencensus/common/internal/random.h"
#include "opencensus/stats/tag_key.h"
#include "opencensus/stats/tag_set.h"

namespace opencensus {
namespace stats {
namespace {

void BM_CreateTagSetInitializerList2(benchmark::State& state) {
  const TagKey key1 = TagKey::Register("tk_1");
  const TagKey key2 = TagKey::Register("tk_2");

  // Test various sortings to avoid optimizing for one.
  const std::initializer_list<std::pair<TagKey, absl::string_view>>
      tag_sets[2] = {{{key1, "value"}, {key2, "value"}},
                     {{key2, "value"}, {key1, "value"}}};

  int iteration = 0;
  for (auto _ : state) {
    // Repeat with different sortings.
    TagSet ts(tag_sets[iteration % 2]);
    ++iteration;
  }
}
BENCHMARK(BM_CreateTagSetInitializerList2);

void BM_CreateTagSetInitializerList4(benchmark::State& state) {
  const TagKey key1 = TagKey::Register("tk_1");
  const TagKey key2 = TagKey::Register("tk_2");
  const TagKey key3 = TagKey::Register("tk_3");
  const TagKey key4 = TagKey::Register("tk_4");

  // Test various sortings to avoid optimizing for one.
  const std::initializer_list<std::pair<TagKey, absl::string_view>>
      tag_sets[4] = {
          {{key1, "value"}, {key2, "value"}, {key3, "value"}, {key4, "value"}},
          {{key4, "value"}, {key3, "value"}, {key2, "value"}, {key1, "value"}},
          {{key2, "value"}, {key4, "value"}, {key3, "value"}, {key1, "value"}},
          {{key3, "value"}, {key1, "value"}, {key2, "value"}, {key4, "value"}}};

  int iteration = 0;
  for (auto _ : state) {
    TagSet ts(tag_sets[iteration % 4]);
    ++iteration;
  }
}
BENCHMARK(BM_CreateTagSetInitializerList4);

void BM_CreateTagSetVector(benchmark::State& state) {
  const int num_keys = state.range(0);
  std::vector<TagKey> tag_keys;
  for (int i = 0; i < num_keys; ++i) {
    tag_keys.push_back(TagKey::Register(absl::StrCat("tk_", i)));
  }

  // Create two randomly-sorted lists of tags.
  constexpr int num_sortings = 8;
  std::vector<std::pair<TagKey, std::string>> tag_sets[num_sortings];
  common::Generator rng(111111);
  for (int i = 0; i < num_sortings; ++i) {
    std::vector<std::pair<uint64_t, TagKey>> keys;
    for (int j = 0; j < num_keys; ++j) {
      keys.emplace_back(rng.Random64(), tag_keys[j]);
    }
    std::sort(keys.begin(), keys.end());
    for (int j = 0; j < num_keys; ++j) {
      tag_sets[i].emplace_back(tag_keys[j], "value");
    }
  }

  int iteration = 0;
  for (auto _ : state) {
    TagSet ts(tag_sets[iteration % num_sortings]);
    ++iteration;
  }
}
BENCHMARK(BM_CreateTagSetVector)->RangeMultiplier(2)->Range(1, 8);

void BM_CopyTagSet(benchmark::State& state) {
  const int num_keys = state.range(0);
  std::vector<std::pair<TagKey, std::string>> tags;
  for (int i = 0; i < num_keys; ++i) {
    tags.emplace_back(TagKey::Register(absl::StrCat("tag_key_", i)), "value");
  }
  const TagSet ts(tags);

  for (auto _ : state) {
    TagSet ts_copy(ts);
  }
}
BENCHMARK(BM_CopyTagSet)->RangeMultiplier(2)->Range(1, 8);

// Set existing tag
void BM_CopyAndSetTag(benchmark::State& state) {
  const int num_keys = state.range(0);
  std::vector<TagKey> tag_keys;
  for (int i = 0; i < num_keys; ++i) {
    tag_keys.push_back(TagKey::Register(absl::StrCat("tk_", i)));
  }

  std::vector<std::pair<TagKey, std::string>> tags;
  for (int i = 0; i < num_keys; ++i) {
    tags.emplace_back(tag_keys[i], "value");
  }
  const TagSet ts(tags);

  int iteration = 0;
  for (auto _ : state) {
    TagSet ts_copy(ts);
    ts_copy.SetTags({{tag_keys[iteration % num_keys], "modified_value"}});
  }
}
BENCHMARK(BM_CopyAndSetTag)->RangeMultiplier(2)->Range(1, 8);

}  // namespace
}  // namespace stats
}  // namespace opencensus

BENCHMARK_MAIN();
