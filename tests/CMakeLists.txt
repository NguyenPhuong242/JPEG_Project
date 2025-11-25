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

