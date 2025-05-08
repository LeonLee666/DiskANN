#pragma once

#include <unordered_map>
#include <vector>
#include <memory>
#include <iostream>
#include <functional>
#include <cmath>
#include <queue>
#include <unordered_set>

namespace diskann {

class GridGraph {
private:
    // 量化相关参数
    static const int QUANTIZATION_DIM = 4;  // 使用前4个维度进行量化
    static const int QUANTIZATION_BITS = 4; // 每个维度4位，共16个区间
    static const int QUANTIZATION_LEVELS = 16; // 2^4 = 16个量化级别
    static const int MAX_NEIGHBORS = 80; // 3^4 - 1 = 80个可能的邻居

    // 成员变量
    std::unordered_map<uint64_t, uint32_t> quantized_coord_map_; // 量化坐标到内部ID的映射
    std::unordered_map<uint32_t, uint64_t> id_to_coord_map_; // 内部ID到量化坐标的映射
    std::unordered_map<uint32_t, std::vector<uint32_t>> neighbors_; // 内部ID到邻居列表的映射
public:
    // 构造函数
    GridGraph() = default;

    // 量化函数
    uint64_t quantizeVector(const void* data_point) const {
        const float* vec = (const float*)data_point;
        uint64_t quantized = 0;
        
        // 对前4个维度进行量化
        for (int i = 0; i < QUANTIZATION_DIM; ++i) {
            // 将值从[0, 256]映射到[0, 15]区间
            float val = vec[i];
            int quantized_val = static_cast<int>((val / 256.0f) * QUANTIZATION_LEVELS);
            quantized_val = std::max(0, std::min(quantized_val, QUANTIZATION_LEVELS - 1));
            
            // 将量化值存储到uint64_t中
            quantized |= (static_cast<uint64_t>(quantized_val) << (i * QUANTIZATION_BITS));
        }
        return quantized;
    }

    // 获取量化坐标的邻居
    std::vector<uint64_t> getNeighborCoords(uint64_t coord) const {
        // 计算实际需要的空间：3^QUANTIZATION_DIM - 1
        size_t num_combinations = 1;  // 使用size_t避免溢出
        for (int i = 0; i < QUANTIZATION_DIM; ++i) {
            num_combinations *= 3;  // 每个维度有3种可能：-1, 0, +1
        }
        std::vector<uint64_t> neighbors;
        neighbors.reserve(num_combinations - 1); // 精确预分配空间

        // 获取每个维度的量化值
        int dim_values[QUANTIZATION_DIM];
        for (int i = 0; i < QUANTIZATION_DIM; ++i) {
            dim_values[i] = (coord >> (i * QUANTIZATION_BITS)) & (QUANTIZATION_LEVELS - 1);
        }

        // 使用迭代方式生成所有可能的组合
        struct State {
            int dim;           // 当前维度
            uint64_t current;  // 当前坐标
            bool has_changed;  // 是否已经改变
            int step;          // 当前步骤：0=不变，1=+1，2=-1
        };
        std::vector<State> stack;
        stack.reserve(QUANTIZATION_DIM * 3);  // 预分配栈空间
        
        // 初始状态
        stack.push_back({0, coord, false, 0});
        
        while (!stack.empty()) {
            State& state = stack.back();
            
            if (state.dim == QUANTIZATION_DIM) {
                // 到达叶子节点
                if (state.has_changed) {
                    neighbors.push_back(state.current);
                }
                stack.pop_back();
                continue;
            }
            
            // 处理当前维度的不同状态
            switch (state.step) {
                case 0:  // 当前维度不变
                    state.step = 1;
                    stack.push_back({state.dim + 1, state.current, state.has_changed, 0});
                    break;
                    
                case 1: {  // 当前维度+1
                    state.step = 2;
                    int new_val = dim_values[state.dim] + 1;
                    if (new_val < QUANTIZATION_LEVELS) {
                        uint64_t next = state.current;
                        next &= ~(static_cast<uint64_t>(QUANTIZATION_LEVELS - 1) << (state.dim * QUANTIZATION_BITS));
                        next |= (static_cast<uint64_t>(new_val) << (state.dim * QUANTIZATION_BITS));
                        stack.push_back({state.dim + 1, next, true, 0});
                    }
                    break;
                }
                    
                case 2: {  // 当前维度-1
                    state.step = 3;
                    int new_val = dim_values[state.dim] - 1;
                    if (new_val >= 0) {
                        uint64_t next = state.current;
                        next &= ~(static_cast<uint64_t>(QUANTIZATION_LEVELS - 1) << (state.dim * QUANTIZATION_BITS));
                        next |= (static_cast<uint64_t>(new_val) << (state.dim * QUANTIZATION_BITS));
                        stack.push_back({state.dim + 1, next, true, 0});
                    }
                    break;
                }
                    
                default:  // 所有状态都处理完了
                    stack.pop_back();
                    break;
            }
        }

        return neighbors;
    }

    // 计算量化坐标之间的距离
    float quantizedDistance(uint64_t coord1, uint64_t coord2) const {
        float distance = 0;
        for (int i = 0; i < QUANTIZATION_DIM; ++i) {
            int val1 = (coord1 >> (i * QUANTIZATION_BITS)) & (QUANTIZATION_LEVELS - 1);
            int val2 = (coord2 >> (i * QUANTIZATION_BITS)) & (QUANTIZATION_LEVELS - 1);
            
            int diff = val1 - val2;
            distance += diff * diff;
        }
        return std::sqrt(distance);
    }

    // 采样函数
    void sample(const void* data_points, size_t n, size_t data_size) {
        size_t sampled_count = 0;
        size_t duplicate_count = 0;
        
        // 遍历所有点进行采样
        for (size_t i = 0; i < n; ++i) {
            const void* data_point = (const char*)data_points + i * data_size;
            uint64_t quantized_coord = quantizeVector(data_point);
            
            // 检查该量化坐标是否已存在
            auto it = quantized_coord_map_.find(quantized_coord);
            if (it == quantized_coord_map_.end()) {
                // 如果不存在，添加新点
                quantized_coord_map_[quantized_coord] = static_cast<uint32_t>(i);
                id_to_coord_map_[static_cast<uint32_t>(i)] = quantized_coord; // 添加反向映射
                sampled_count++;
            } else {
                duplicate_count++;
            }
        }

    }

    // 构建图结构
    void buildGraph() {
        // 遍历所有第1层的点
        for (const auto& pair : quantized_coord_map_) {
            uint64_t quantized_coord = pair.first;
            uint32_t internal_id = pair.second;
            
            // 获取邻居坐标
            std::vector<uint64_t> neighbor_coords = getNeighborCoords(quantized_coord);
            std::vector<uint32_t> node_neighbors;
            node_neighbors.reserve(MAX_NEIGHBORS);
            
            // 收集有效的邻居
            for (uint64_t coord : neighbor_coords) {
                auto it = quantized_coord_map_.find(coord);
                if (it != quantized_coord_map_.end()) {
                    // 计算量化坐标之间的距离
                    float distance = quantizedDistance(quantized_coord, coord);
                    // 如果距离小于等于2，则建立边
                    if (distance <= 2.0) {
                        node_neighbors.push_back(it->second);
                    }
                }
            }
            
            // 存储邻居列表
            neighbors_.emplace(internal_id, std::move(node_neighbors));
        }
    }

    bool isConnected() {
        if (quantized_coord_map_.empty()) return true;  // 空图视为连通
        
        // 使用BFS检查连通性
        std::unordered_set<uint32_t> visited;
        std::queue<uint32_t> q;
        
        // 从第一个节点开始BFS
        uint32_t start_id = quantized_coord_map_.begin()->second;
        q.push(start_id);
        visited.insert(start_id);
        
        while (!q.empty()) {
            uint32_t curr = q.front();
            q.pop();
            
            // 遍历当前节点的所有邻居
            auto neighbors_it = neighbors_.find(curr);
            if (neighbors_it != neighbors_.end()) {
                for (uint32_t neighbor : neighbors_it->second) {
                    if (visited.find(neighbor) == visited.end()) {
                        visited.insert(neighbor);
                        q.push(neighbor);
                    }
                }
            }
        }
        
        // 如果访问过的节点数等于总节点数,则图是连通的
        return visited.size() == quantized_coord_map_.size();
    }

    // 搜索函数
    std::vector<uint32_t> search(const void* query_data, size_t k) const {
        // 量化查询点
        uint64_t query_coord = quantizeVector(query_data);
        
        // 使用优先队列存储候选点（距离越小优先级越高）
        std::priority_queue<std::pair<float, uint32_t>, 
                          std::vector<std::pair<float, uint32_t>>,
                          std::greater<std::pair<float, uint32_t>>> candidates;
        
        // 使用bitmap记录已访问的点
        size_t max_id = 0;
        for (const auto& pair : quantized_coord_map_) {
            max_id = std::max(max_id, pair.first);
        }
        std::vector<uint64_t> visited_bitmap((max_id + 64) >> 6, 0);  // 向上取整到64的倍数
        
        // 使用优先队列存储结果（距离越大优先级越高）
        std::priority_queue<std::pair<float, uint32_t>> results;
        
        // 获取初始邻居坐标
        std::vector<uint64_t> neighbor_coords = getNeighborCoords(query_coord);
        
        // 将初始邻居加入候选队列
        for (uint64_t coord : neighbor_coords) {
            auto it = quantized_coord_map_.find(coord);
            if (it != quantized_coord_map_.end()) {
                uint32_t internal_id = static_cast<uint32_t>(it->second);
                if (internal_id < (visited_bitmap.size() << 6)) {  // 检查是否在范围内
                    float distance = quantizedDistance(query_coord, coord);
                    candidates.push(std::make_pair(distance, internal_id));
                    // 设置bitmap
                    visited_bitmap[internal_id >> 6] |= (1ULL << (internal_id & 63));
                }
            }
        }
        
        // 开始best first search
        while (!candidates.empty()) {
            auto current = candidates.top();
            candidates.pop();
            
            float current_dist = current.first;
            uint32_t current_id = current.second;
            
            if (results.size() >= k && current_dist > results.top().first) {
                break;
            }
            
            results.push(current);
            if (results.size() > k) {
                results.pop();
            }
            
            // 探索当前点的邻居
            auto neighbors_it = neighbors_.find(current_id);
            if (neighbors_it != neighbors_.end()) {
                for (size_t neighbor_id : neighbors_it->second) {
                    uint32_t neighbor_id_32 = static_cast<uint32_t>(neighbor_id);
                    if (neighbor_id_32 < (visited_bitmap.size() << 6)) {  // 检查是否在范围内
                        // 检查bitmap
                        if (!(visited_bitmap[neighbor_id_32 >> 6] & (1ULL << (neighbor_id_32 & 63)))) {
                            // 设置bitmap
                            visited_bitmap[neighbor_id_32 >> 6] |= (1ULL << (neighbor_id_32 & 63));
                            // 使用find替代at，避免边界检查
                            auto coord_it = id_to_coord_map_.find(neighbor_id_32);
                            if (coord_it != id_to_coord_map_.end()) {
                                float neighbor_dist = quantizedDistance(query_coord, coord_it->second);
                                candidates.push(std::make_pair(neighbor_dist, neighbor_id_32));
                            }
                        }
                    }
                }
            }
        }
        
        // 收集结果
        std::vector<uint32_t> final_results;
        final_results.reserve(k);
        while (!results.empty()) {
            final_results.push_back(results.top().second);
            results.pop();
        }
        std::reverse(final_results.begin(), final_results.end());
        
        return final_results;
    }

    // 获取量化坐标映射
    const std::unordered_map<uint64_t, uint32_t>& getQuantizedCoordMap() const {
        return quantized_coord_map_;
    }

    // 获取邻居关系
    const std::unordered_map<uint32_t, std::vector<uint32_t>>& getNeighbors() const {
        return neighbors_;
    }
};
} // namespace diskann
