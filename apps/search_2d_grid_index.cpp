// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <iomanip>
#include <algorithm>
#include <numeric>

#include "index.h"
#include "utils.h"
#include "defaults.h"

void print_usage(char *argv0) {
    std::cout << "用法: " << argv0 << " --index_prefix <索引前缀> --query_file <查询文件> [options]" << std::endl;
    std::cout << "参数说明:" << std::endl;
    std::cout << "  --index_prefix <前缀>: 预先构建的索引文件路径前缀" << std::endl;
    std::cout << "  --query_file <文件>: 二进制格式的查询文件 (.bin)" << std::endl;
    std::cout << "选项:" << std::endl;
    std::cout << "  --search_L <值1,值2,...>: 搜索时的L值列表 (默认: 50,100,150)" << std::endl;
    std::cout << "  --K <值>: 返回的邻居数 (默认: 10)" << std::endl;
    std::cout << "  --test_queries <值>: 测试查询数量 (默认: 1024)" << std::endl;
    std::cout << "  --gt_file <文件>: ground truth文件路径 (可选，用于计算recall)" << std::endl;
    std::cout << "  --num_threads <值>: 搜索线程数 (默认: 1)" << std::endl;
    std::cout << std::endl;
    std::cout << "示例: " << argv0 << " --index_prefix test_index --query_file queries.bin --search_L 50,100,150,200 --K 10 --test_queries 1000 --gt_file gt.bin" << std::endl;
}

std::vector<uint32_t> parse_search_L(const std::string& L_str) {
    std::vector<uint32_t> L_vec;
    std::string current = "";
    
    for (char c : L_str) {
        if (c == ',') {
            if (!current.empty()) {
                L_vec.push_back(std::stoul(current));
                current = "";
            }
        } else {
            current += c;
        }
    }
    
    if (!current.empty()) {
        L_vec.push_back(std::stoul(current));
    }
    
    return L_vec;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        print_usage(argv[0]);
        return -1;
    }

    std::string index_prefix = "";
    std::string query_file = "";
    
    // 默认搜索参数
    std::vector<uint32_t> search_L_vec = {50, 100, 150};
    uint32_t K = 10;
    uint32_t test_queries = 1024;
    std::string gt_file = "";
    uint32_t num_threads = 1;

    // 解析命令行参数
    for (int i = 1; i < argc - 1; i++) {
        std::string arg = argv[i];
        if (arg == "--index_prefix") {
            index_prefix = argv[++i];
        } else if (arg == "--query_file") {
            query_file = argv[++i];
        } else if (arg == "--search_L") {
            search_L_vec = parse_search_L(argv[++i]);
        } else if (arg == "--K") {
            K = std::stoul(argv[++i]);
        } else if (arg == "--test_queries") {
            test_queries = std::stoul(argv[++i]);
        } else if (arg == "--gt_file") {
            gt_file = argv[++i];
        } else if (arg == "--num_threads") {
            num_threads = std::stoul(argv[++i]);
        }
    }

    // 检查必需参数
    if (index_prefix.empty() || query_file.empty()) {
        std::cerr << "错误: 必须指定 --index_prefix 和 --query_file 参数" << std::endl;
        print_usage(argv[0]);
        return -1;
    }

    std::cout << "2D Grid-Aware DiskANN 索引搜索测试程序" << std::endl;
    std::cout << "搜索参数: K=" << K << ", 测试查询数=" << test_queries << ", threads=" << num_threads << ", L值: ";
    for (size_t i = 0; i < search_L_vec.size(); i++) {
        std::cout << search_L_vec[i];
        if (i < search_L_vec.size() - 1) std::cout << ",";
    }
    std::cout << std::endl;

    try {
        // 检查索引文件是否存在（DiskANN内存索引的图文件没有扩展名）
        std::string graph_file = index_prefix;
        std::string data_file = index_prefix + ".data";
        if (!file_exists(graph_file)) {
            std::cerr << "错误: 索引图文件不存在: " << graph_file << std::endl;
            std::cerr << "请确保已使用 build_2d_grid_index 构建了索引" << std::endl;
            return -1;
        }
        if (!file_exists(data_file)) {
            std::cerr << "错误: 索引数据文件不存在: " << data_file << std::endl;
            std::cerr << "请确保已使用 build_2d_grid_index 构建了索引" << std::endl;
            return -1;
        }

        // 读取查询文件元信息
        size_t query_num, query_dim;
        diskann::get_bin_metadata(query_file, query_num, query_dim);
        std::cout << "查询集: " << query_num << " 点, " << query_dim << " 维" << std::endl;
        
        if (query_dim != 2) {
            std::cerr << "错误: 查询数据必须是2维" << std::endl;
            return -1;
        }

        test_queries = std::min(test_queries, (uint32_t)query_num);

        // 创建搜索参数
        auto search_params = std::make_shared<diskann::IndexSearchParams>(
            *std::max_element(search_L_vec.begin(), search_L_vec.end()), 
            num_threads
        );

        // 创建索引对象并加载
        diskann::Index<uint8_t, uint32_t, uint32_t> index(
            diskann::Metric::L2, 
            query_dim, 
            0,      // max_points (will be set when loading)
            nullptr, // 构建参数
            search_params,
            0,       // frozen points
            false,   // dynamic index
            false,   // enable tags
            false,   // concurrent consolidate
            false,   // pq dist build
            0,       // num pq chunks
            false,   // use opq
            false    // filtered index
        );

        std::cout << "加载索引: " << index_prefix << std::endl;
        index.load(index_prefix.c_str(), num_threads, search_L_vec[0]);
        std::cout << "索引加载完成" << std::endl;

        // 读取查询数据
        uint8_t *query_data = nullptr;
        size_t query_file_num, query_file_dim;
        diskann::load_bin<uint8_t>(query_file, query_data, query_file_num, query_file_dim);

        // 加载ground truth（如果提供）
        uint32_t *gt_ids = nullptr;
        float *gt_dists = nullptr;
        size_t gt_num = 0, gt_dim = 0;
        bool calc_recall_flag = false;
        
        if (!gt_file.empty() && file_exists(gt_file)) {
            diskann::load_truthset(gt_file, gt_ids, gt_dists, gt_num, gt_dim);
            if (gt_num != query_num) {
                std::cerr << "警告: ground truth查询数量 (" << gt_num << ") 与查询文件不匹配 (" << query_num << ")" << std::endl;
                gt_num = std::min(gt_num, query_num);
            }
            // 确保test_queries不超过ground truth的数量
            test_queries = std::min(test_queries, (uint32_t)gt_num);
            calc_recall_flag = true;
            std::cout << "已加载ground truth文件，将计算前" << test_queries << "个查询的recall@" << K << std::endl;
        } else if (!gt_file.empty()) {
            std::cout << "警告: 未找到ground truth文件 " << gt_file << "，跳过recall计算" << std::endl;
        }

        std::cout << "\n开始搜索测试..." << std::endl;
        std::cout.setf(std::ios_base::fixed, std::ios_base::floatfield);
        std::cout.precision(2);
        
        std::cout << std::setw(6) << "L" 
                  << std::setw(12) << "QPS"
                  << std::setw(18) << "Avg Dist Cmps"
                  << std::setw(20) << "Mean Latency (us)"
                  << std::setw(18) << "99.9 Latency (us)";
        if (calc_recall_flag) {
            std::cout << std::setw(15) << ("Recall@" + std::to_string(K));
        }
        std::cout << std::endl;
        
        int table_width = calc_recall_flag ? 89 : 74;
        std::cout << std::string(table_width, '=') << std::endl;

        // 对每个L值进行测试
        for (uint32_t L : search_L_vec) {
            if (L < K) {
                std::cout << std::setw(6) << L << "   [跳过：L < K]" << std::endl;
                continue;
            }

            // 预热
            {
                uint32_t indices[K];
                float distances[K];
                index.search(query_data, K, L, indices, distances);
            }
            
            // 分配结果存储空间，参照search_memory_index的方式
            std::vector<uint32_t> query_result_ids(K * test_queries);
            std::vector<float> latency_stats(test_queries);
            std::vector<uint32_t> cmp_stats(test_queries);
            
            // 计算QPS - 只测量纯搜索时间
            auto qps_start = std::chrono::high_resolution_clock::now();
            for (uint32_t i = 0; i < test_queries; i++) {
                uint32_t indices[K];
                float distances[K];
                index.search(query_data + i * query_dim, K, L, indices, distances);
            }
            auto qps_end = std::chrono::high_resolution_clock::now();
            auto total_search_time = std::chrono::duration_cast<std::chrono::duration<double>>(qps_end - qps_start);
            double qps = test_queries / total_search_time.count();
            
            // 收集详细统计信息和结果（用于recall计算）
            for (uint32_t i = 0; i < test_queries; i++) {
                auto query_start = std::chrono::high_resolution_clock::now();
                auto [hops, cmps] = index.search(query_data + i * query_dim, K, L, 
                                                query_result_ids.data() + i * K, nullptr);
                auto query_end = std::chrono::high_resolution_clock::now();
                
                auto query_time = std::chrono::duration_cast<std::chrono::nanoseconds>(query_end - query_start);
                latency_stats[i] = query_time.count() / 1000.0f; // 转换为微秒
                cmp_stats[i] = cmps;
            }
            
            std::sort(latency_stats.begin(), latency_stats.end());
            
            double mean_latency = std::accumulate(latency_stats.begin(), latency_stats.end(), 0.0) / test_queries;
            float percentile_99_9 = latency_stats[(uint64_t)(0.999 * test_queries)];
            float avg_cmps = std::accumulate(cmp_stats.begin(), cmp_stats.end(), 0.0f) / test_queries;
            
            // 计算recall（如果有ground truth）
            double recall = 0.0;
            if (calc_recall_flag) {
                // 使用与search_memory_index相同的方式计算recall
                recall = diskann::calculate_recall((uint32_t)test_queries, gt_ids, gt_dists, (uint32_t)gt_dim,
                                                   query_result_ids.data(), K, K);
            }
            
            std::cout << std::setw(6) << L
                      << std::setw(12) << qps
                      << std::setw(18) << avg_cmps
                      << std::setw(20) << mean_latency
                      << std::setw(18) << percentile_99_9;
            if (calc_recall_flag) {
                std::cout << std::setw(15) << recall;
            }
            std::cout << std::endl;
        }

        std::cout << std::string(table_width, '=') << std::endl;
        std::cout << "搜索测试完成！" << std::endl;

        // 清理内存
        delete[] query_data;
        if (gt_ids) delete[] gt_ids;
        if (gt_dists) delete[] gt_dists;

        return 0;

    } catch (const std::exception &e) {
        std::cerr << "错误: " << e.what() << std::endl;
        return -1;
    }
} 