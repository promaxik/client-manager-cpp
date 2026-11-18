cmake_minimum_required(VERSION 3.20)
project(ClientManager)
 
set(CMAKE_CXX_STANDARD 17)
 
add_executable(ClientManager main.cpp)
 
set(PostgresSQL_ROOT "C:/Program Files/PostgreSQL/18")
set(PostgreSQL_ADDITIONAL_VERSIONS "18")
add_subdirectory(C:/libs/libpqxx-8.0.2 libpqxx-build)
 
target_link_libraries(ClientManager pqxx)