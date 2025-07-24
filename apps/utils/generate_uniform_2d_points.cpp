// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

#include <iostream>
#include <fstream>
#include <vector>
#include <random>
#include <cstring>
#include <string>

// 生成均匀分布的二维坐标点和查询点
// 数据空间: 256x256 (uint8范围: 0-255)
// 数据点数: 10240 (Grid划分: 32x32，每个grid约包含10个点)
// 查询点数: 1024 (每个grid单元1个查询点)

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cout << "用法: " << argv[0] << " <数据输出文件名> <查询输出文件名>" << std::endl;
        std::cout << "例如: " << argv[0] << " uniform_2d_points.bin uniform_2d_queries.bin" << std::endl;
        return -1;
    }

    const std::string output_file = argv[1];
    const std::string query_file = argv[2];
    const uint32_t total_points = 10240;
    const uint32_t dimensions = 2;
    const uint32_t grid_size = 32;  // 32x32 grid
    const uint32_t grid_cell_size = 256 / grid_size;  // 每个grid单元的大小 = 8
    const uint32_t points_per_grid = total_points / (grid_size * grid_size);  // 每个grid约10个点
    
    std::cout << "生成参数:" << std::endl;
    std::cout << "  总点数: " << total_points << std::endl;
    std::cout << "  维度: " << dimensions << std::endl;
    std::cout << "  Grid大小: " << grid_size << "x" << grid_size << std::endl;
    std::cout << "  每个grid单元大小: " << grid_cell_size << "x" << grid_cell_size << std::endl;
    std::cout << "  每个grid约点数: " << points_per_grid << std::endl;
    
    // 创建随机数生成器
    std::random_device rd;
    std::mt19937 gen(rd());
    
    // 存储所有生成的点
    std::vector<uint8_t> points;
    points.reserve(total_points * dimensions);
    
    uint32_t points_generated = 0;
    
    // 遍历每个grid单元
    for (uint32_t grid_y = 0; grid_y < grid_size; grid_y++) {
        for (uint32_t grid_x = 0; grid_x < grid_size; grid_x++) {
            // 计算当前grid单元的边界
            uint32_t min_x = grid_x * grid_cell_size;
            uint32_t max_x = std::min(255u, (grid_x + 1) * grid_cell_size - 1);
            uint32_t min_y = grid_y * grid_cell_size;
            uint32_t max_y = std::min(255u, (grid_y + 1) * grid_cell_size - 1);
            
            // 计算该grid中要生成的点数
            uint32_t remaining_points = total_points - points_generated;
            uint32_t remaining_grids = (grid_size * grid_size) - (grid_y * grid_size + grid_x);
            uint32_t points_in_this_grid = (remaining_points + remaining_grids - 1) / remaining_grids;
            
            // 在当前grid中生成点
            std::uniform_int_distribution<uint32_t> x_dist(min_x, max_x);
            std::uniform_int_distribution<uint32_t> y_dist(min_y, max_y);
            
            for (uint32_t i = 0; i < points_in_this_grid && points_generated < total_points; i++) {
                uint8_t x = static_cast<uint8_t>(x_dist(gen));
                uint8_t y = static_cast<uint8_t>(y_dist(gen));
                
                points.push_back(x);
                points.push_back(y);
                points_generated++;
            }
        }
    }
    
    std::cout << "实际生成点数: " << points_generated << std::endl;
    
    // 验证数据分布
    std::vector<std::vector<uint32_t>> grid_counts(grid_size, std::vector<uint32_t>(grid_size, 0));
    for (uint32_t i = 0; i < points_generated; i++) {
        uint8_t x = points[i * 2];
        uint8_t y = points[i * 2 + 1];
        uint32_t grid_x = x / grid_cell_size;
        uint32_t grid_y = y / grid_cell_size;
        if (grid_x >= grid_size) grid_x = grid_size - 1;
        if (grid_y >= grid_size) grid_y = grid_size - 1;
        grid_counts[grid_y][grid_x]++;
    }
    
    // 统计分布
    uint32_t min_count = UINT32_MAX, max_count = 0;
    double total_count = 0;
    for (uint32_t y = 0; y < grid_size; y++) {
        for (uint32_t x = 0; x < grid_size; x++) {
            uint32_t count = grid_counts[y][x];
            min_count = std::min(min_count, count);
            max_count = std::max(max_count, count);
            total_count += count;
        }
    }
    double avg_count = total_count / (grid_size * grid_size);
    
    std::cout << "分布统计:" << std::endl;
    std::cout << "  平均每个grid点数: " << avg_count << std::endl;
    std::cout << "  最小点数: " << min_count << std::endl;
    std::cout << "  最大点数: " << max_count << std::endl;
    
    // 写入二进制文件
    std::ofstream writer(output_file, std::ios::binary);
    if (!writer.is_open()) {
        std::cerr << "错误: 无法创建输出文件 " << output_file << std::endl;
        return -1;
    }
    
    // 写入文件头：点数和维度数
    writer.write(reinterpret_cast<const char*>(&points_generated), sizeof(uint32_t));
    writer.write(reinterpret_cast<const char*>(&dimensions), sizeof(uint32_t));
    
    // 写入数据
    writer.write(reinterpret_cast<const char*>(points.data()), points.size() * sizeof(uint8_t));
    
    writer.close();
    
    std::cout << "数据写入完成: " << output_file << std::endl;
    std::cout << "文件大小: " << (8 + points.size()) << " 字节" << std::endl;
    
    // 生成查询点 - 每个grid单元一个query点
    std::cout << "\n开始生成查询点..." << std::endl;
    
    const uint32_t num_queries = grid_size * grid_size;  // 1024个查询点
    std::vector<uint8_t> queries;
    queries.reserve(num_queries * dimensions);
    
    // 为每个grid单元生成一个query点
    for (uint32_t grid_y = 0; grid_y < grid_size; grid_y++) {
        for (uint32_t grid_x = 0; grid_x < grid_size; grid_x++) {
            // 计算当前grid单元的边界
            uint32_t min_x = grid_x * grid_cell_size;
            uint32_t max_x = std::min(255u, (grid_x + 1) * grid_cell_size - 1);
            uint32_t min_y = grid_y * grid_cell_size;
            uint32_t max_y = std::min(255u, (grid_y + 1) * grid_cell_size - 1);
            
            // 在当前grid中随机生成一个查询点
            std::uniform_int_distribution<uint32_t> x_dist(min_x, max_x);
            std::uniform_int_distribution<uint32_t> y_dist(min_y, max_y);
            
            uint8_t query_x = static_cast<uint8_t>(x_dist(gen));
            uint8_t query_y = static_cast<uint8_t>(y_dist(gen));
            
            queries.push_back(query_x);
            queries.push_back(query_y);
        }
    }
    
    std::cout << "生成查询点数: " << num_queries << std::endl;
    
    // 写入查询文件
    std::ofstream query_writer(query_file, std::ios::binary);
    if (!query_writer.is_open()) {
        std::cerr << "错误: 无法创建查询文件 " << query_file << std::endl;
        return -1;
    }
    
    // 写入查询文件头：查询数和维度数
    query_writer.write(reinterpret_cast<const char*>(&num_queries), sizeof(uint32_t));
    query_writer.write(reinterpret_cast<const char*>(&dimensions), sizeof(uint32_t));
    
    // 写入查询数据
    query_writer.write(reinterpret_cast<const char*>(queries.data()), queries.size() * sizeof(uint8_t));
    
    query_writer.close();
    
    std::cout << "查询文件写入完成: " << query_file << std::endl;
    std::cout << "查询文件大小: " << (8 + queries.size()) << " 字节" << std::endl;
    
    // 验证查询点分布
    std::cout << "\n查询点分布验证:" << std::endl;
    std::vector<std::vector<uint32_t>> query_grid_counts(grid_size, std::vector<uint32_t>(grid_size, 0));
    for (uint32_t i = 0; i < num_queries; i++) {
        uint8_t x = queries[i * 2];
        uint8_t y = queries[i * 2 + 1];
        uint32_t grid_x = x / grid_cell_size;
        uint32_t grid_y = y / grid_cell_size;
        if (grid_x >= grid_size) grid_x = grid_size - 1;
        if (grid_y >= grid_size) grid_y = grid_size - 1;
        query_grid_counts[grid_y][grid_x]++;
    }
    
    // 检查是否每个grid都有正好一个查询点
    bool all_grids_have_one_query = true;
    for (uint32_t y = 0; y < grid_size; y++) {
        for (uint32_t x = 0; x < grid_size; x++) {
            if (query_grid_counts[y][x] != 1) {
                all_grids_have_one_query = false;
                std::cout << "  警告: Grid(" << x << "," << y << ") 有 " << query_grid_counts[y][x] << " 个查询点" << std::endl;
            }
        }
    }
    
    if (all_grids_have_one_query) {
        std::cout << "  ✓ 每个grid单元都有且仅有一个查询点" << std::endl;
    }
    
    std::cout << "\n生成完成!" << std::endl;
    std::cout << "数据文件: " << output_file << " (包含 " << points_generated << " 个数据点)" << std::endl;
    std::cout << "查询文件: " << query_file << " (包含 " << num_queries << " 个查询点)" << std::endl;
    
    return 0;
} 