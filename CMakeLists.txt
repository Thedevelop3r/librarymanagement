cmake_minimum_required(VERSION 3.29)
project(librarymanagement)

set(CMAKE_CXX_STANDARD 20)

add_executable(librarymanagement main.cpp)

find_package(SqliteOrm CONFIG REQUIRED)
target_link_libraries(librarymanagement PRIVATE sqlite_orm::sqlite_orm)
find_package(unofficial-sqlite3 CONFIG REQUIRED)
target_link_libraries(librarymanagement PRIVATE unofficial::sqlite3::sqlite3)

