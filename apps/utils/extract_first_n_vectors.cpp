// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>
#include <iomanip>
#include <vector>
#include "utils.h"

// 从bin文件中提取前n个向量
int main(int argc, char **argv)
{
    if (argc != 5)
    {
        std::cout << "用法: " << argv[0] << " <数据类型> <输入bin文件> <输出bin文件> <向量数量n>" << std::endl;
        std::cout << "数据类型: float/int8/uint8" << std::endl;
        std::cout << "例如: " << argv[0] << " uint8 input.bin output.bin 1000" << std::endl;
        return -1;
    }

    std::string data_type = argv[1];
    std::string input_file = argv[2];
    std::string output_file = argv[3];
    int32_t n = std::stoi(argv[4]);

    if (n <= 0)
    {
        std::cerr << "错误: 向量数量n必须大于0" << std::endl;
        return -1;
    }

    // 确定数据类型大小
    int datasize;
    if (data_type == "float")
    {
        datasize = sizeof(float);
    }
    else if (data_type == "int8" || data_type == "uint8")
    {
        datasize = sizeof(uint8_t);
    }
    else
    {
        std::cerr << "错误: 不支持的数据类型 '" << data_type << "'. 请使用 float/int8/uint8" << std::endl;
        return -1;
    }

    // 打开输入文件
    std::ifstream reader(input_file, std::ios::binary);
    if (!reader.is_open())
    {
        std::cerr << "错误: 无法打开输入文件 " << input_file << std::endl;
        return -1;
    }

    // 读取文件头
    int32_t original_npts, ndims;
    reader.read(reinterpret_cast<char*>(&original_npts), sizeof(int32_t));
    reader.read(reinterpret_cast<char*>(&ndims), sizeof(int32_t));

    if (!reader.good())
    {
        std::cerr << "错误: 无法读取文件头" << std::endl;
        reader.close();
        return -1;
    }

    std::cout << "输入文件信息:" << std::endl;
    std::cout << "  原始点数: " << original_npts << std::endl;
    std::cout << "  维度数: " << ndims << std::endl;
    std::cout << "  数据类型: " << data_type << " (" << datasize << " 字节/元素)" << std::endl;

    // 检查n是否超过原始点数
    if (n > original_npts)
    {
        std::cout << "警告: 请求的向量数量 (" << n << ") 超过文件中的向量数量 (" << original_npts << ")" << std::endl;
        std::cout << "将提取所有 " << original_npts << " 个向量" << std::endl;
        n = original_npts;
    }

    std::cout << "将提取前 " << n << " 个向量" << std::endl;

    // 打开输出文件
    std::ofstream writer(output_file, std::ios::binary);
    if (!writer.is_open())
    {
        std::cerr << "错误: 无法创建输出文件 " << output_file << std::endl;
        reader.close();
        return -1;
    }

    // 写入新的文件头
    writer.write(reinterpret_cast<const char*>(&n), sizeof(int32_t));        // 更新点数为n
    writer.write(reinterpret_cast<const char*>(&ndims), sizeof(int32_t));    // 维度数保持不变

    if (!writer.good())
    {
        std::cerr << "错误: 无法写入输出文件头" << std::endl;
        reader.close();
        writer.close();
        return -1;
    }

    // 计算需要复制的数据大小
    size_t vector_size = ndims * datasize;  // 每个向量的字节数
    size_t total_data_size = n * vector_size;  // 总共需要复制的字节数

    std::cout << "复制数据:" << std::endl;
    std::cout << "  每个向量大小: " << vector_size << " 字节" << std::endl;
    std::cout << "  总数据大小: " << total_data_size << " 字节" << std::endl;

    // 分块复制数据，避免内存占用过大
    const size_t BUFFER_SIZE = 1024 * 1024;  // 1MB缓冲区
    std::vector<uint8_t> buffer(BUFFER_SIZE);
    
    size_t bytes_copied = 0;
    size_t bytes_remaining = total_data_size;

    while (bytes_remaining > 0)
    {
        size_t bytes_to_copy = std::min(bytes_remaining, BUFFER_SIZE);
        
        // 读取数据
        reader.read(reinterpret_cast<char*>(buffer.data()), bytes_to_copy);
        size_t bytes_read = reader.gcount();
        
        if (bytes_read != bytes_to_copy)
        {
            std::cerr << "错误: 读取数据时发生错误. 期望 " << bytes_to_copy << " 字节, 实际读取 " << bytes_read << " 字节" << std::endl;
            reader.close();
            writer.close();
            return -1;
        }

        // 写入数据
        writer.write(reinterpret_cast<const char*>(buffer.data()), bytes_read);
        if (!writer.good())
        {
            std::cerr << "错误: 写入数据时发生错误" << std::endl;
            reader.close();
            writer.close();
            return -1;
        }

        bytes_copied += bytes_read;
        bytes_remaining -= bytes_read;

        // 显示进度
        if (total_data_size > 1024 * 1024)  // 只在大文件时显示进度
        {
            double progress = static_cast<double>(bytes_copied) / total_data_size * 100.0;
            std::cout << "\r复制进度: " << std::fixed << std::setprecision(1) << progress << "% (" 
                      << bytes_copied << " / " << total_data_size << " 字节)" << std::flush;
        }
    }

    if (total_data_size > 1024 * 1024)
    {
        std::cout << std::endl;  // 换行
    }

    reader.close();
    writer.close();

    std::cout << "提取完成!" << std::endl;
    std::cout << "输出文件: " << output_file << std::endl;
    std::cout << "包含 " << n << " 个向量，每个向量 " << ndims << " 维" << std::endl;
    
    // 计算文件大小
    size_t output_file_size = 8 + total_data_size;  // 8字节头部 + 数据
    std::cout << "输出文件大小: " << output_file_size << " 字节";
    if (output_file_size > 1024 * 1024)
    {
        std::cout << " (" << std::fixed << std::setprecision(2) << static_cast<double>(output_file_size) / (1024 * 1024) << " MB)";
    }
    else if (output_file_size > 1024)
    {
        std::cout << " (" << std::fixed << std::setprecision(2) << static_cast<double>(output_file_size) / 1024 << " KB)";
    }
    std::cout << std::endl;

    return 0;
} 