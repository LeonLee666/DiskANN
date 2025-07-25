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

// PQ-based Grid-aware index building parameters  
const uint32_t PQ_GRID_SIZE = 256;          // 256x256 grid from PQ quantization (0-255 per dimension)

// Configurable stage ranges for grid-aware building (Chebyshev distance)
const uint32_t STAGE1_GRID_RANGE = 16;       // Stage 1: grid distance ≤ 1 (e.g., 3x3)
const uint32_t STAGE2_GRID_RANGE = 32;       // Stage 2: grid distance ≤ 2 (e.g., 5x5) 
const uint32_t STAGE3_GRID_RANGE = 64;       // Stage 3: grid distance ≤ 3 (e.g., 7x7)

// Stage-wise neighbor selection limits for PQ grid-aware building
const uint32_t PQ_STAGE1_MAX_NEIGHBORS = 16;    // Stage 1 max neighbors
const uint32_t PQ_STAGE2_MAX_NEIGHBORS = 8;    // Stage 2 max neighbors  
const uint32_t PQ_STAGE3_MAX_NEIGHBORS = 4;    // Stage 3 max neighbors

// Search parameters for each stage
const uint32_t PQ_STAGE1_SEARCH_LIST_SIZE = 500;
const uint32_t PQ_STAGE2_SEARCH_LIST_SIZE = 500;
const uint32_t PQ_STAGE3_SEARCH_LIST_SIZE = 500;
} // namespace defaults
} // namespace diskann
