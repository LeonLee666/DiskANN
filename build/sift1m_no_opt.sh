rm -rf res*

input_data=../../../datasets/sift/sift_base.fbin
query_data=../../../datasets/sift/sift_query.fbin
gt_data=../../../datasets/sift/sift_query_base_gt100
index_prefix=sift1m_idx
result=res

sudo sysctl -w vm.drop_caches=3
sleep 3

./apps/search_disk_index --data_type float --dist_fn l2 \
        --index_path_prefix ${index_prefix} \
        --query_file ${query_data} \
        --gt_file ${gt_data} -K 10 \
        -L 10 20 30 40 50 60 70 80 90 100 \
        --result_path ${result} \
        --num_nodes_to_cache 0 -W 8 --use_exact_distance