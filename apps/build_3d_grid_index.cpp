// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

#include <iostream>
#include <string>
#include <chrono>

#include "index.h"
#include "utils.h"
#include "defaults.h"

void print_usage(char *argv0) {
    std::cout << "用法: " << argv0 << " --data_file <数据文件> --index_prefix <输出索引前缀> [options]" << std::endl;
    std::cout << "参数说明:" << std::endl;
    std::cout << "  --data_file <文件>: 二进制格式的数据文件 (.bin)" << std::endl;
    std::cout << "  --index_prefix <前缀>: 索引保存的路径前缀" << std::endl;
    std::cout << "选项:" << std::endl;
    std::cout << "  --R <值>: 图的最大度数 (默认: 32)" << std::endl;
    std::cout << "  --build_L <值>: 构建时的候选列表大小 (默认: 100)" << std::endl;
    std::cout << "  --alpha <值>: RNG剪枝参数 (默认: 1.2)" << std::endl;
    std::cout << "  --num_threads <值>: 线程数 (默认: 1)" << std::endl;
    std::cout << std::endl;
    std::cout << "示例: " << argv0 << " --data_file data.bin --index_prefix test_index --R 32 --build_L 100 --alpha 1.2 --num_threads 8" << std::endl;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        print_usage(argv[0]);
        return -1;
    }

    std::string data_file = "";
    std::string index_prefix = "";
    
    // 默认构建参数
    uint32_t R = 32;
    uint32_t build_L = 100;
    float alpha = 1.2f;
    uint32_t num_threads = 1;

    // 解析命令行参数
    for (int i = 1; i < argc - 1; i++) {
        std::string arg = argv[i];
        if (arg == "--data_file") {
            data_file = argv[++i];
        } else if (arg == "--index_prefix") {
            index_prefix = argv[++i];
        } else if (arg == "--R") {
            R = std::stoul(argv[++i]);
        } else if (arg == "--build_L") {
            build_L = std::stoul(argv[++i]);
        } else if (arg == "--alpha") {
            alpha = std::stof(argv[++i]);
        } else if (arg == "--num_threads") {
            num_threads = std::stoul(argv[++i]);
        }
    }

    // 检查必需参数
    if (data_file.empty() || index_prefix.empty()) {
        std::cerr << "错误: 必须指定 --data_file 和 --index_prefix 参数" << std::endl;
        print_usage(argv[0]);
        return -1;
    }

    std::cout << "3D Grid-Aware DiskANN 索引构建程序" << std::endl;
    std::cout << "构建参数: R=" << R << ", L=" << build_L << ", alpha=" << alpha << ", threads=" << num_threads << std::endl;
    std::cout << "注意: Grid-Aware模式下build_L参数不起作用，使用独立的阶段搜索列表大小" << std::endl;

    try {
        // 读取数据文件元信息
        size_t data_num, data_dim;
        diskann::get_bin_metadata(data_file, data_num, data_dim);
        
        std::cout << "数据集: " << data_num << " 点, " << data_dim << " 维" << std::endl;
        
        if (data_dim != 3) {
            std::cerr << "错误: Grid-aware 3D建图只支持3维数据" << std::endl;
            return -1;
        }

        // 创建索引构建参数
        auto index_build_params = diskann::IndexWriteParametersBuilder(build_L, R)
                                      .with_alpha(alpha)
                                      .with_saturate_graph(false)
                                      .with_num_threads(num_threads)
                                      .build();

        // 创建索引
        diskann::Index<uint8_t, uint32_t, uint32_t> index(
            diskann::Metric::L2, 
            data_dim, 
            data_num,
            std::make_shared<diskann::IndexWriteParameters>(index_build_params),
            nullptr, // 搜索参数
            0,       // frozen points
            false,   // dynamic index
            false,   // enable tags
            false,   // concurrent consolidate
            false,   // pq dist build
            0,       // num pq chunks
            false,   // use opq
            false    // filtered index
        );

        std::cout << "开始构建索引..." << std::endl;
        std::cout << "Grid设置: " << diskann::defaults::GRID_SIZE_3D << "x" << diskann::defaults::GRID_SIZE_3D << "x" << diskann::defaults::GRID_SIZE_3D 
                  << " grid (每个单元 " << diskann::defaults::GRID_CELL_SIZE_3D << "x" << diskann::defaults::GRID_CELL_SIZE_3D << "x" << diskann::defaults::GRID_CELL_SIZE_3D << " voxels)" << std::endl;
        std::cout << "三阶段邻居数限制: Stage1=" << diskann::defaults::STAGE1_MAX_NEIGHBORS_3D 
                  << ", Stage2=" << diskann::defaults::STAGE2_MAX_NEIGHBORS_3D 
                  << ", Stage3=" << diskann::defaults::STAGE3_MAX_NEIGHBORS_3D << std::endl;
        std::cout << "注意：构建过程中会显示度数统计等详细信息" << std::endl;
        std::cout.flush();
        
        auto build_start = std::chrono::high_resolution_clock::now();
        index.build(data_file.c_str(), data_num);
        auto build_end = std::chrono::high_resolution_clock::now();
        auto build_time = std::chrono::duration_cast<std::chrono::milliseconds>(build_end - build_start);
        
        std::cout << "\n索引构建完成，耗时: " << build_time.count() << " ms" << std::endl;

        // 统计图结构信息
        std::cout << "\n图结构统计:" << std::endl;
        index.print_status();
        index.count_nodes_at_bfs_levels();

        std::cout << "\n保存索引到: " << index_prefix << ".*" << std::endl;
        index.save(index_prefix.c_str());
        std::cout << "索引保存完成！" << std::endl;

        return 0;

    } catch (const std::exception &e) {
        std::cerr << "错误: " << e.what() << std::endl;
        return -1;
    }
} 