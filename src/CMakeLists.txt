# MiniDB library
file(GLOB_RECURSE MINIDB_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/storage/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/index/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/catalog/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/sql/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/optimizer/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/execution/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/txn/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/recovery/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/extension/*.cpp"
)

add_library(minidb_lib STATIC ${MINIDB_SOURCES})
target_include_directories(minidb_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# MiniDB executable
add_executable(minidb main.cpp)
target_link_libraries(minidb PRIVATE minidb_lib pthread)
