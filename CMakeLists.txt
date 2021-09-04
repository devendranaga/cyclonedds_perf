project(performance_dds)

cmake_minimum_required(VERSION 3.10)

set(SRC
    ./perf.c
    ./performance.c)

set(SRC_SUB
    ./perf.c
    ./perf_subscriber.c)

include_directories(./
                    /usr/local/include/)

add_executable(perf_pub_dds ${SRC})
target_link_libraries(perf_pub_dds ddsc)

add_executable(perf_sub_dds ${SRC_SUB})
target_link_libraries(perf_sub_dds ddsc)
