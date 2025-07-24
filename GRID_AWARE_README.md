# Grid-Aware 2D索引构建功能

本功能为DiskANN新增了针对二维坐标数据的grid-aware索引构建方法，通过分阶段的稀疏化选点策略来优化图结构。

## 功能概述

### 设计理念
- **局部性优先**: 优先连接近距离的点
- **距离分层**: 不同距离范围使用不同的采样策略  
- **稀疏化控制**: 每一层都可以独立控制邻居数量
- **RNG剪枝**: 每个阶段内部使用相对邻居图(RNG)策略进行剪枝

### 分阶段建图策略
1. **Stage 1 (3×3 grid)**: 以当前节点为中心的3×3 grid范围内稀疏化选择邻居
2. **Stage 2 (4×4 外围)**: 在4×4 grid范围但不在3×3范围内的区域选择邻居
3. **Stage 3 (5×5 外围)**: 在5×5 grid范围但不在4×4范围内的区域选择邻居

## 配置参数

### Grid配置 (在`include/defaults.h`中定义)
```cpp
const uint32_t GRID_SIZE_2D = 32;           // 32×32 grid划分
const uint32_t GRID_CELL_SIZE_2D = 8;       // 每个grid单元8×8像素

// 各阶段邻居数量限制
const uint32_t STAGE1_MAX_NEIGHBORS = 8;    // Stage 1最大邻居数
const uint32_t STAGE2_MAX_NEIGHBORS = 6;    // Stage 2最大邻居数  
const uint32_t STAGE3_MAX_NEIGHBORS = 4;    // Stage 3最大邻居数

// 各阶段搜索参数
const uint32_t STAGE1_SEARCH_LIST_SIZE = 50;
const uint32_t STAGE2_SEARCH_LIST_SIZE = 40;
const uint32_t STAGE3_SEARCH_LIST_SIZE = 30;
```

### 使用条件
- **数据类型**: 必须是`uint8_t`
- **数据维度**: 必须是2维
- **数据范围**: 坐标值在0-255之间
- **过滤索引**: 不能与过滤索引同时使用

## 数据生成工具

### 生成均匀分布的2D数据
```bash
cd apps/utils
./generate_uniform_2d_points data.bin queries.bin
```

该工具会生成：
- `data.bin`: 包含10,240个均匀分布的2D数据点
- `queries.bin`: 包含1,024个查询点(每个grid单元一个)

### 数据格式
二进制文件格式：
```
[4字节: 点数量][4字节: 维度数][数据: 所有点的坐标]
```

## 索引构建与测试

### 基本使用
```bash
cd apps
./test_2d_grid_index data.bin queries.bin output_index [R] [L] [alpha] [threads]
```

参数说明：
- `data.bin`: 输入数据文件
- `queries.bin`: 查询文件  
- `output_index`: 输出索引前缀
- `R`: 图的最大度数 (默认32)
- `L`: 构建时候选列表大小 (默认100)
- `alpha`: RNG剪枝参数 (默认1.2)
- `threads`: 线程数 (默认1)

### 示例
```bash
# 生成测试数据
./apps/utils/generate_uniform_2d_points test_data.bin test_queries.bin

# 构建grid-aware索引
./apps/test_2d_grid_index test_data.bin test_queries.bin grid_index 32 100 1.2 4

# 运行结果示例
=== 2D Grid-Aware DiskANN索引测试 ===
数据文件: test_data.bin
查询文件: test_queries.bin
索引前缀: grid_index
参数: R=32, L=100, alpha=1.2, threads=4

数据集信息:
  点数: 10240
  维度: 2

Grid配置:
  Grid大小: 32x32
  Grid单元大小: 8x8
  阶段1邻居数: 8
  阶段2邻居数: 6
  阶段3邻居数: 4

开始构建索引...
索引构建完成，耗时: 1250 毫秒
索引保存完成，耗时: 45 毫秒

查询测试:
  查询数量: 1024
  查询维度: 2
执行搜索测试 (K=10, L=50)...
查询 0: 1205(12.5) 3421(15.8) 7832(18.2) ...
...

搜索性能统计:
  平均查询时间: 15.6 微秒
  总搜索时间: 156 毫秒
  QPS: 6410.25

=== 测试完成 ===
索引文件已保存到: grid_index.*
```

## 代码集成

### 自动检测使用
当满足以下条件时，DiskANN会自动使用grid-aware建图：
- 数据类型为`uint8_t`
- 数据维度为2
- 非过滤索引

### 手动控制
如需手动控制，可以修改`src/index.cpp`中的条件：
```cpp
bool use_grid_aware = (_dim == 2 && std::is_same_v<T, uint8_t> && !_filtered_index);
```

## 性能特点

### 优势
- **局部性保证**: 确保每个点都与其网格邻居有连接
- **分层结构**: 多层次的连接模式提供更好的搜索路径
- **可调参数**: 每个阶段的邻居数量可独立调整
- **RNG剪枝**: 使用成熟的相对邻居图策略保证图质量

### 适用场景
- 2D地理坐标数据
- 图像像素坐标  
- 2D游戏世界坐标
- 其他具有空间局部性的2维数据

## 自定义配置

可以通过修改`include/defaults.h`中的参数来调整grid-aware建图的行为：

1. **调整grid大小**: 修改`GRID_SIZE_2D`和`GRID_CELL_SIZE_2D`
2. **调整邻居数量**: 修改`STAGE*_MAX_NEIGHBORS`
3. **调整搜索列表**: 修改`STAGE*_SEARCH_LIST_SIZE`

## 注意事项

1. **内存使用**: grid-aware建图可能在某些阶段使用更多内存
2. **构建时间**: 由于多阶段搜索，构建时间可能稍长于传统方法
3. **数据分布**: 最适合空间分布相对均匀的数据
4. **边界处理**: 靠近边界的点会自动处理grid范围越界问题

## 故障排除

### 常见错误
1. **"Grid-aware building only supports uint8_t data type"**
   - 确保使用`uint8_t`数据类型

2. **"Grid-aware building only supports 2-dimensional data"**
   - 确保数据维度为2

3. **构建速度慢**
   - 尝试增加线程数
   - 减少各阶段的搜索列表大小

### 调试建议
- 使用小数据集先验证功能
- 检查数据格式是否正确
- 确认grid配置参数合理 