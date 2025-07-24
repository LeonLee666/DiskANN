# Grid-Aware 2D索引构建功能

本功能为DiskANN新增了针对二维坐标数据的grid-aware索引构建方法，通过分阶段的稀疏化选点策略来优化图结构。

## 功能概述

### 设计理念
- **局部性优先**: 优先连接近距离的点
- **距离分层**: 不同距离范围使用不同的采样策略  
- **稀疏化控制**: 每一层都可以独立控制邻居数量
- **RNG剪枝**: 每个阶段内部使用相对邻居图(RNG)策略进行剪枝

### 分阶段建图策略
1. **Stage 1 (3×3 grid)**: 在3×3 grid完整范围内（grid distance ≤ 1）创建最多4条边
2. **Stage 2 (5×5 外围)**: 在5×5 grid范围但排除3×3区域（1 < grid distance ≤ 2）创建最多3条边
3. **Stage 3 (7×7 外围)**: 在7×7 grid范围但排除5×5区域（2 < grid distance ≤ 3）创建最多2条边

## 配置参数

### Grid配置 (在`include/defaults.h`中定义)
```cpp
const uint32_t GRID_SIZE_2D = 32;           // 32×32 grid划分
const uint32_t GRID_CELL_SIZE_2D = 8;       // 每个grid单元8×8像素

// 各阶段邻居数量限制
const uint32_t STAGE1_MAX_NEIGHBORS = 4;    // Stage 1: 3×3网格内最大邻居数
const uint32_t STAGE2_MAX_NEIGHBORS = 3;    // Stage 2: 5×5但非3×3区域最大邻居数  
const uint32_t STAGE3_MAX_NEIGHBORS = 2;    // Stage 3: 7×7但非5×5区域最大邻居数

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

### 推荐使用：分离式构建和搜索

从v2.0开始，我们提供了分离的构建和搜索程序，提供更好的灵活性和可维护性：

#### 1. 构建索引
```bash
cd apps
./build_2d_grid_index --data_file <数据文件> --index_prefix <输出索引前缀> [options]
```

构建参数：
- `--data_file <文件>`: 输入数据文件
- `--index_prefix <前缀>`: 输出索引前缀
- `--R <值>`: 图的最大度数 (默认32)
- `--build_L <值>`: 构建时候选列表大小 (默认100) **注意：在Grid-Aware模式下此参数无效**
- `--alpha <值>`: RNG剪枝参数 (默认1.2)
- `--num_threads <值>`: 线程数 (默认1)

**重要说明**: 在Grid-Aware 2D构建中，`--build_L`参数不起作用，因为Grid-Aware使用基于几何的搜索策略而不是传统的图遍历。实际的搜索列表大小由`STAGE*_SEARCH_LIST_SIZE`常量控制。

#### 2. 搜索测试
```bash
cd apps
./search_2d_grid_index --index_prefix <索引前缀> --query_file <查询文件> [options]
```

搜索参数：
- `--index_prefix <前缀>`: 预先构建的索引文件前缀
- `--query_file <文件>`: 查询文件
- `--search_L <值1,值2,...>`: 搜索时的L值列表 (默认50,100,150)
- `--K <值>`: 返回的邻居数 (默认10)
- `--test_queries <值>`: 测试查询数量 (默认1024)
- `--gt_file <文件>`: ground truth文件 (可选，用于计算recall)
- `--num_threads <值>`: 搜索线程数 (默认1)

### 完整使用示例

#### 步骤1：生成测试数据
```bash
# 生成测试数据
./apps/utils/generate_uniform_2d_points test_data.bin test_queries.bin
```

#### 步骤2：构建索引
```bash
# 构建grid-aware索引
./apps/build_2d_grid_index --data_file test_data.bin --index_prefix grid_index --R 32 --build_L 100 --alpha 1.2 --num_threads 8

# 输出示例:
2D Grid-Aware DiskANN 索引构建程序
构建参数: R=32, L=100, alpha=1.2, threads=8
数据集: 10240 点, 2 维
开始构建索引...
注意：构建过程中会显示度数统计等详细信息

Starting index build with 10240 points...
...
Index built with degree: max:32  avg:24.5  min:18  count(deg<2):0

索引构建完成，耗时: 1250 ms

图结构统计:
Max points: 10240, Cur points: 10240, Frozen points: 1, start: 9999
Graph max degree: 32, Avg degree: 24.5, Effective max degree: 30
Number of edges: 125440

保存索引到: grid_index.*
索引保存完成！

# 构建后会生成以下文件：
# grid_index      - 图结构文件（主文件）
# grid_index.data - 数据文件
# grid_index.tags - 标签文件（如果使用标签）
```

#### 步骤3：搜索测试
```bash
# 执行搜索测试
./apps/search_2d_grid_index --index_prefix grid_index --query_file test_queries.bin --search_L 50,100,150,200 --K 10 --test_queries 1000

# 输出示例:
2D Grid-Aware DiskANN 索引搜索测试程序
搜索参数: K=10, 测试查询数=1000, threads=1, L值: 50,100,150,200
查询集: 1024 点, 2 维
加载索引: grid_index
索引加载完成

开始搜索测试...
     L          QPS     Avg Dist Cmps   Mean Latency (us)  99.9 Latency (us)
================================================================================
    50      6410.25              12.8               156.2               245.6
   100      5234.12              18.4               191.0               298.4
   150      4521.33              24.2               221.1               334.8
   200      4098.76              29.6               244.0               367.2
================================================================================
搜索测试完成！
```

### 传统方式：一体化测试（向后兼容）
```bash
cd apps
./test_2d_grid_index --data_file data.bin --query_file queries.bin --index_prefix output_index [options]
```

这个程序包含构建和搜索的完整流程，但对于大规模实验，建议使用分离式程序。

### 分离式程序的优势

1. **灵活性**: 可以多次测试不同搜索参数而无需重新构建索引
2. **效率**: 构建一次索引，进行多次搜索实验
3. **可维护性**: 构建和搜索逻辑分离，便于调试和优化
4. **批量实验**: 便于进行大规模参数扫描实验
5. **资源管理**: 构建和搜索可以在不同的机器上进行

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