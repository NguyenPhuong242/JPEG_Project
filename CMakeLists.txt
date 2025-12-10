cmake_minimum_required(VERSION 3.22)
project(JPEG_Compressor LANGUAGES CXX)

# Bắt buộc dùng C++17 cho cấu trúc code mới
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# -------------------------------------------------------------
# 1. Định nghĩa thư viện lõi (JPEG Core)
# -------------------------------------------------------------

# Tìm tất cả các file .cpp trong thư mục src/dct, src/quantification, src/core
# Lưu ý: Không dùng GLOB_RECURSE bừa bãi để tránh lấy nhầm main.cpp hoặc file rác
file(GLOB LIB_SOURCES 
    "src/dct/*.cpp"
    "src/quantification/*.cpp"
    "src/core/*.cpp"
    # Nếu em có file entropy/rle.cpp thì thêm dòng dưới, nếu đã xóa thì bỏ qua
    # "src/entropy/*.cpp" 
)

# Tạo thư viện tĩnh 'jpeg_core' từ các file nguồn trên
add_library(jpeg_core STATIC ${LIB_SOURCES})

# Chỉ định thư mục chứa header (.h)
target_include_directories(jpeg_core PUBLIC ${PROJECT_SOURCE_DIR}/include)

# -------------------------------------------------------------
# 2. Định nghĩa file chạy chính (Executable)
# -------------------------------------------------------------

# Tạo file chạy 'jpeg_cli' từ src/main.cpp
add_executable(jpeg_cli main.cpp)

# Liên kết file chạy với thư viện lõi
target_link_libraries(jpeg_cli PRIVATE jpeg_core)

# -------------------------------------------------------------
# 3. Cấu hình Test (Giữ nguyên của em)
# -------------------------------------------------------------
enable_testing()
# Kiểm tra xem thư mục tests có tồn tại không trước khi add
if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)
    add_subdirectory(tests)
endif()