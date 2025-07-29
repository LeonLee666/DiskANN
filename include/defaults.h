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


const uint32_t STAGE1_MAX_NEIGHBORS = 10;    // Stage 1 max neighbors from top 64

const uint32_t STAGE2_SEARCH_LIST_SIZE = 60;    
const uint32_t STAGE2_MAX_NEIGHBORS = 10;    // Stage 2 max neighbors from rank 65-128  
const float STAGE2_ALPHA = 2.0f;    // Stage 2 alpha for medium-quality candidates  


const uint32_t STAGE3_START_RANK = 60;          // Stage 3 starts from rank 301 (index 300)
const uint32_t STAGE3_SEARCH_LIST_SIZE = 400;    // Stage 3: extended to rank 350 for more candidates
const uint32_t STAGE3_MAX_NEIGHBORS = 5;    // Stage 3 max neighbors from rank 129-256
const float STAGE3_ALPHA = 1.0f;    // Stage 3 alpha for lower-quality candidates (less selective)


} // namespace defaults
} // namespace diskann
