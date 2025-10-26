cmake_minimum_required(VERSION 3.16)
project(waybar-cffi-modules VERSION 1.0.0 LANGUAGES CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 设置构建类型
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# 编译选项
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g")

# 查找依赖
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)

# 查找scdoc用于生成manpage
find_program(SCDOC_EXECUTABLE scdoc)
if(NOT SCDOC_EXECUTABLE)
    message(WARNING "scdoc not found. Man pages will not be generated.")
endif()

# 查找或获取nlohmann/json
find_package(nlohmann_json QUIET)
if(NOT nlohmann_json_FOUND)
    # 如果系统没有安装，使用FetchContent获取
    include(FetchContent)
    FetchContent_Declare(
        json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.11.2
    )
    FetchContent_MakeAvailable(json)
    set(JSON_LIBRARIES nlohmann_json::nlohmann_json)
else()
    set(JSON_LIBRARIES nlohmann_json::nlohmann_json)
endif()

# 创建接口库以复用通用配置
add_library(waybar_common INTERFACE)

# 设置通用编译选项
target_compile_options(waybar_common INTERFACE
    -Wall
    -Wextra
    -Wpedantic
    -Wconversion
    -Wsign-conversion
)

# 设置通用包含目录
target_include_directories(waybar_common INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${GTK3_INCLUDE_DIRS}
)

# 设置通用链接库
target_link_libraries(waybar_common INTERFACE
    ${GTK3_LIBRARIES}
    ${JSON_LIBRARIES}
    m
)

# 设置通用目标属性
set_target_properties(waybar_common PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)

# 源文件
set(COMMON_SOURCES
    src/common.cpp
)

# 头文件
set(COMMON_HEADERS
    include/common.hpp
    include/module_base.hpp
)

# 定义所有模块
set(MODULES cpu rapl temperature gpu network)

# 循环创建所有模块
foreach(module ${MODULES})
    # 创建模块共享库
    add_library(${module}_module SHARED
        ${COMMON_SOURCES}
        src/modules/${module}_module.cpp
        ${COMMON_HEADERS}
        include/modules/${module}_module.hpp
    )
    
    # 链接通用接口库
    target_link_libraries(${module}_module PRIVATE waybar_common)
    
    # 设置模块目标属性
    set_target_properties(${module}_module PROPERTIES
        OUTPUT_NAME "${module}"
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
    )
endforeach()

# 处理manpage
if(SCDOC_EXECUTABLE)
    set(MANPAGE_MODULES cpu rapl temperature gpu network)
    
    # 为每个模块创建manpage
    foreach(module ${MANPAGE_MODULES})
        # 设置输入输出文件
        set(MANPAGE_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/docs/waybar-cffi-${module}.5.scd")
        set(MANPAGE_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/waybar-cffi-${module}.5")
        
        # 添加自定义命令来生成manpage
        add_custom_command(
            OUTPUT ${MANPAGE_OUTPUT}
            COMMAND ${SCDOC_EXECUTABLE} < ${MANPAGE_SOURCE} > ${MANPAGE_OUTPUT}
            DEPENDS ${MANPAGE_SOURCE}
            COMMENT "Generating man page for waybar-cffi-${module}"
            VERBATIM
        )
        
        # 收集所有manpage输出文件
        list(APPEND MANPAGE_OUTPUTS ${MANPAGE_OUTPUT})
    endforeach()
    
    # 添加自定义目标
    add_custom_target(manpage ALL DEPENDS ${MANPAGE_OUTPUTS})
    
    message(STATUS "Man pages will be generated")
else()
    message(STATUS "Skipping man page generation (scdoc not found)")
endif()

# 调试信息
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "GTK3 version: ${GTK3_VERSION}")