cmake_minimum_required(VERSION 3.15)
project(SecureChat CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- PAKETLER ---
find_package(gRPC CONFIG REQUIRED)
find_package(Protobuf REQUIRED)
find_package(Threads REQUIRED)
find_package(PostgreSQL REQUIRED)

# pqxx için pkg-config kullan
find_package(PkgConfig REQUIRED)
pkg_check_modules(PQXX REQUIRED libpqxx)

set(_PROTOBUF_LIBPROTOBUF protobuf::libprotobuf)
set(_REFLECTION gRPC::grpc++_reflection)
set(_GRPC_GRPCPP gRPC::grpc++)

# --- YOLLAR ---
set(PROTO_SRC_DIR "${CMAKE_SOURCE_DIR}/protos")
set(INC_DIR "${CMAKE_SOURCE_DIR}/inc")
set(SRC_DIR "${CMAKE_SOURCE_DIR}/src")
set(PROTO_FILE "${PROTO_SRC_DIR}/auth.proto")

# --- HEDEF DOSYALAR ---
set(AUTH_PB_H "${INC_DIR}/auth.pb.h")
set(AUTH_GRPC_H "${INC_DIR}/auth.grpc.pb.h")
set(AUTH_PB_CC "${SRC_DIR}/auth.pb.cc")
set(AUTH_GRPC_CC "${SRC_DIR}/auth.grpc.pb.cc")

# --- PROTOC KOMUTU ---
add_custom_command(
      OUTPUT "${AUTH_PB_H}" "${AUTH_GRPC_H}" "${AUTH_PB_CC}" "${AUTH_GRPC_CC}"
      
      COMMAND protobuf::protoc
      ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
           --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
           -I "${PROTO_SRC_DIR}"
           --plugin=protoc-gen-grpc=$<TARGET_FILE:gRPC::grpc_cpp_plugin>
           "${PROTO_FILE}"

      COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_BINARY_DIR}/auth.pb.h" "${INC_DIR}/auth.pb.h"
      COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_BINARY_DIR}/auth.grpc.pb.h" "${INC_DIR}/auth.grpc.pb.h"
      COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_BINARY_DIR}/auth.pb.cc" "${SRC_DIR}/auth.pb.cc"
      COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_BINARY_DIR}/auth.grpc.pb.cc" "${SRC_DIR}/auth.grpc.pb.cc"

      DEPENDS "${PROTO_FILE}")

# --- KÜTÜPHANE OLUŞTURMA ---
add_library(auth_lib
  "${AUTH_PB_CC}"
  "${AUTH_GRPC_CC}"
  "${AUTH_PB_H}"
  "${AUTH_GRPC_H}"
)

target_link_libraries(auth_lib
  ${_REFLECTION}
  ${_GRPC_GRPCPP}
  ${_PROTOBUF_LIBPROTOBUF}
  ${PostgreSQL_LIBRARIES}
  ${PQXX_LIBRARIES}
)

target_include_directories(auth_lib PRIVATE
  ${PostgreSQL_INCLUDE_DIRS}
  ${PQXX_INCLUDE_DIRS}
  ${INC_DIR}
)

# --- INCLUDE DİZİNLERİ ---
include_directories(${INC_DIR})

# --- SERVER ---
add_executable(chat_server 
  src/main.cpp 
  src/TokenManager.cpp
  src/ChatSession.cpp
  src/ChatServer.cpp
  src/AuthService.cpp
  src/AdminService.cpp
  src/ChatService.cpp
  src/DataBaseManager.cpp
)

target_link_libraries(chat_server 
  auth_lib 
  Threads::Threads
  ${PostgreSQL_LIBRARIES}
  ${PQXX_LIBRARIES}
)

target_include_directories(chat_server PRIVATE 
  ${PostgreSQL_INCLUDE_DIRS}
  ${PQXX_INCLUDE_DIRS}
  ${INC_DIR}
)

# --- CLIENT (Şimdilik client.cpp yok, ileride eklenecek) ---
# add_executable(chat_client src/client.cpp)
# target_link_libraries(chat_client auth_lib Threads::Threads ${PostgreSQL_LIBRARIES} ${PQXX_LIBRARIES})
# target_include_directories(chat_client PRIVATE ${PostgreSQL_INCLUDE_DIRS} ${PQXX_INCLUDE_DIRS} ${INC_DIR})
