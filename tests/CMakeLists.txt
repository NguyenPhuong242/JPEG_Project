add_executable(testdct test_dct.cpp)
target_include_directories(testdct PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(testdct PRIVATE jpeg_core)
add_test(NAME testdct COMMAND testdct)

add_executable(testquant test_quantification.cpp)
target_include_directories(testquant PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(testquant PRIVATE jpeg_core)
add_test(NAME testquant COMMAND testquant)

add_executable(testrle test_rle.cpp)
target_include_directories(testrle PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(testrle PRIVATE jpeg_core)
add_test(NAME testrle COMMAND testrle)

add_executable(testhuffman test_huffman.cpp)
target_include_directories(testhuffman PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(testhuffman PRIVATE jpeg_core)
add_test(NAME testhuffman COMMAND testhuffman)

add_executable(testcompression test_compression.cpp)
target_include_directories(testcompression PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(testcompression PRIVATE jpeg_core)
add_test(NAME testcompression COMMAND testcompression)

add_executable(testdecompression test_decompression.cpp)
target_include_directories(testdecompression PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(testdecompression PRIVATE jpeg_core)
add_test(NAME testdecompression COMMAND testdecompression)

add_executable(testpgm dat_to_pgm.cpp)
target_include_directories(testpgm PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(testpgm PRIVATE jpeg_core)
add_test(NAME testpgm COMMAND testpgm)