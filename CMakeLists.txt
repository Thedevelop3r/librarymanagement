cmake_minimum_required(VERSION 3.29)
project(librarymanagement)

set(CMAKE_CXX_STANDARD 20)

# Add the directory containing SqliteOrmConfig.cmake to CMAKE_PREFIX_PATH
set(CMAKE_PREFIX_PATH "C:/Users/mrbil/.vcpkg-clion/vcpkg (1)/installed/x64-mingw-dynamic")

# Add the executable
add_executable(librarymanagement main.cpp)

# Find and link SQLite ORM
find_package(SqliteOrm CONFIG REQUIRED)
target_link_libraries(librarymanagement PRIVATE sqlite_orm::sqlite_orm)

# Find and link SQLite3
find_package(unofficial-sqlite3 CONFIG REQUIRED)
target_link_libraries(librarymanagement PRIVATE unofficial::sqlite3::sqlite3)
