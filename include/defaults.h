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

// Grid-aware 2D index building parameters
const uint32_t GRID_SIZE_2D = 32;           // 32x32 grid for 256x256 space
const uint32_t GRID_CELL_SIZE_2D = 8;       // Each grid cell is 8x8 pixels

// Stage-wise neighbor selection limits for 2D grid-aware building
const uint32_t STAGE1_MAX_NEIGHBORS = 3;    // 3x3 grid neighbors (grid distance ≤ 1)
const uint32_t STAGE2_MAX_NEIGHBORS = 3;    // 5x5 but exclude 3x3 area (1 < grid distance ≤ 2)
const uint32_t STAGE3_MAX_NEIGHBORS = 3;    // 7x7 but exclude 5x5 area (2 < grid distance ≤ 3)

// Search parameters for each stage
const uint32_t STAGE1_SEARCH_LIST_SIZE = 90;
const uint32_t STAGE2_SEARCH_LIST_SIZE = 160;
const uint32_t STAGE3_SEARCH_LIST_SIZE = 240;
} // namespace defaults
} // namespace diskann
