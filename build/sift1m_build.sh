rm -rf sift1m_idx*

sleep 3
R=64
L=125
input_data=../../../datasets/sift/sift_base.fbin
index_prefix=sift1m_idx

./apps/build_disk_index --data_type float --dist_fn l2 \
        --data_path ${input_data} \
        --index_path_prefix ${index_prefix} \
        -R $R -L $L -B 0.02 -M 50