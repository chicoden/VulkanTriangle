cmake_minimum_required(VERSION 3.20)
project(VulkanTriangle)

set(CMAKE_CXX_STANDARD 17)
message(STATUS "Using C++17")

add_executable(${PROJECT_NAME} "VulkanTriangle.cpp")

target_include_directories(${PROJECT_NAME} PUBLIC "C:/glfw/include")
target_link_directories(${PROJECT_NAME} PUBLIC "C:/glfw/lib")
target_link_libraries(${PROJECT_NAME} glfw3)

target_include_directories(${PROJECT_NAME} PUBLIC "C:/VulkanSDK/1.3.290.0/Include")
target_link_directories(${PROJECT_NAME} PUBLIC "C:/VulkanSDK/1.3.290.0/Lib")
target_link_libraries(${PROJECT_NAME} vulkan-1)

target_include_directories(${PROJECT_NAME} PUBLIC "C:/glm")