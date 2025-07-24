// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

#pragma once
#include <stdint.h>

namespace diskann
{
namespace defaults
{
const float ALPHA = 1.2f;
const uint32_t NUM_THREADS = 0;
const uint32_t MAX_OCCLUSION_SIZE = 750;
const bool HAS_LABELS = false;
const uint32_t FILTER_LIST_SIZE = 0;
const uint32_t NUM_FROZEN_POINTS_STATIC = 0;
const uint32_t NUM_FROZEN_POINTS_DYNAMIC = 1;

// In-mem index related limits
const float GRAPH_SLACK_FACTOR = 1.3f;

// SSD Index related limits
const uint64_t MAX_GRAPH_DEGREE = 512;
const uint64_t SECTOR_LEN = 4096;
const uint64_t MAX_N_SECTOR_READS = 128;

// following constants should always be specified, but are useful as a
// sensible default at cli / python boundaries
const uint32_t MAX_DEGREE = 64;
const uint32_t BUILD_LIST_SIZE = 100;
const uint32_t SATURATE_GRAPH = false;
const uint32_t SEARCH_LIST_SIZE = 100;

// Stage-wise neighbor selection limits for ranking-based building
const uint32_t STAGE1_MAX_NEIGHBORS = 20;    // Stage 1 max neighbors from top 64
const uint32_t STAGE2_MAX_NEIGHBORS = 20;    // Stage 2 max neighbors from rank 65-128  
const uint32_t STAGE3_MAX_NEIGHBORS = 20;    // Stage 3 max neighbors from rank 129-256

// Multi-stage candidate selection based on search ranking
const uint32_t STAGE1_SEARCH_LIST_SIZE = 40;     // Stage 1: top 64 candidates
const uint32_t STAGE2_SEARCH_LIST_SIZE = 100;    // Stage 2: top 128 candidates (rank 65-128)
const uint32_t STAGE3_SEARCH_LIST_SIZE = 300;    // Stage 3: top 256 candidates (rank 129-256)


} // namespace defaults
} // namespace diskann
