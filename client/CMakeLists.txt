# ═══════════════════════════════════════════════════════════════════════════
#                         SECURE CHAT CLIENT - CMAKE
# Qt6 + gRPC tabanlı chat istemcisi build dosyası
# ═══════════════════════════════════════════════════════════════════════════

cmake_minimum_required(VERSION 3.16)
project(SecureChatClient LANGUAGES CXX)

# C++20 standardı
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt MOC, UIC, RCC otomatik
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# ═══════════════════════════════════════════════════════════════════════════
#                         PAKETLERİ BUL
# ═══════════════════════════════════════════════════════════════════════════

# Qt6 (Widgets, Network ve Concurrent modülleri)
find_package(Qt6 REQUIRED COMPONENTS Widgets Network Concurrent)

# gRPC ve Protobuf
find_package(gRPC CONFIG REQUIRED)
find_package(Protobuf REQUIRED)
find_package(Threads REQUIRED)

# ═══════════════════════════════════════════════════════════════════════════
#                         PROTOBUF KODLARI
# ═══════════════════════════════════════════════════════════════════════════

set(_PROTOBUF_LIBPROTOBUF protobuf::libprotobuf)
set(_REFLECTION gRPC::grpc++_reflection)
set(_GRPC_GRPCPP gRPC::grpc++)

# Proto dosyası yolu
set(PROTO_SRC_DIR "${CMAKE_SOURCE_DIR}/../protos")
set(PROTO_FILE "${PROTO_SRC_DIR}/auth.proto")

# Oluşturulacak dosyalar
set(PROTO_SRCS "${CMAKE_CURRENT_BINARY_DIR}/auth.pb.cc")
set(PROTO_HDRS "${CMAKE_CURRENT_BINARY_DIR}/auth.pb.h")
set(GRPC_SRCS "${CMAKE_CURRENT_BINARY_DIR}/auth.grpc.pb.cc")
set(GRPC_HDRS "${CMAKE_CURRENT_BINARY_DIR}/auth.grpc.pb.h")

# Protoc komutları
add_custom_command(
    OUTPUT "${PROTO_SRCS}" "${PROTO_HDRS}" "${GRPC_SRCS}" "${GRPC_HDRS}"
    COMMAND protobuf::protoc
    ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
         --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
         -I "${PROTO_SRC_DIR}"
         --plugin=protoc-gen-grpc=$<TARGET_FILE:gRPC::grpc_cpp_plugin>
         "${PROTO_FILE}"
    DEPENDS "${PROTO_FILE}"
    COMMENT "Generating gRPC/Protobuf files..."
)

# ═══════════════════════════════════════════════════════════════════════════
#                         EXECUTABLE
# ═══════════════════════════════════════════════════════════════════════════

# Kaynak dosyaları
set(SOURCES
    main.cpp
    MainWindow.cpp
    MainWindow.hpp
    ${PROTO_SRCS}
    ${GRPC_SRCS}
)

# Çalıştırılabilir dosya oluştur
add_executable(${PROJECT_NAME} ${SOURCES})

# Include dizinleri
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

# ═══════════════════════════════════════════════════════════════════════════
#                         BAĞLANTILAR (LINKING)
# ═══════════════════════════════════════════════════════════════════════════

target_link_libraries(${PROJECT_NAME} PRIVATE
    # Qt kütüphaneleri
    Qt6::Widgets
    Qt6::Network
    Qt6::Concurrent
    
    # gRPC kütüphaneleri
    ${_REFLECTION}
    ${_GRPC_GRPCPP}
    ${_PROTOBUF_LIBPROTOBUF}
    
    # Thread kütüphanesi
    Threads::Threads
)
