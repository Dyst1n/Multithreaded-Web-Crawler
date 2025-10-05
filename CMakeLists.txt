cmake_minimum_required(VERSION 3.16)
project(WebCrawler VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


find_package(CURL REQUIRED)


include_directories(src)


file(GLOB SOURCES "src/*.cpp")


add_executable(web_crawler ${SOURCES})


target_link_libraries(web_crawler PRIVATE CURL::libcurl)
