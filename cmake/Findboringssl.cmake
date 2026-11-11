message("Find boringssl")
if(NOT SSL_PATH AND DEFINED ENV{MQAS_BORINGSSL_ROOT})
    set(SSL_PATH "$ENV{MQAS_BORINGSSL_ROOT}" CACHE PATH "Root of the BoringSSL installation used by mqas" FORCE)
endif()
set(SSL_PATH "${SSL_PATH}" CACHE PATH "Root of the BoringSSL installation used by mqas")
if(NOT SSL_PATH)
    message("No SSL_PATH find")
    find_path(boringssl_INCLUDE_DIR openssl REQUIRED)
    find_library(boringssl_ssl_LIBRARY ssl REQUIRED)
    find_library(boringssl_crypto_LIBRARY crypto REQUIRED)
    set(boringssl_LIBRARY
        ${boringssl_ssl_LIBRARY}
        ${boringssl_crypto_LIBRARY}
    )
    
    set(VCPKG_BINARAY_DIR "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin")

    find_file(boringssl_ssl_BINARY "ssl${CMAKE_SHARED_LIBRARY_SUFFIX}" PATHS ${VCPKG_BINARAY_DIR} REQUIRED)
    find_file(boringssl_crypto_BINARY "crypto${CMAKE_SHARED_LIBRARY_SUFFIX}" PATHS ${VCPKG_BINARAY_DIR} REQUIRED)
    set(boringssl_BINARY
        ${boringssl_ssl_BINARY}
        ${boringssl_crypto_BINARY}
    )

else()
    message("${SSL_PATH}/include")
    # Never combine headers, import libraries or runtime DLLs from different
    # BoringSSL installations (BoringSSL does not promise ABI compatibility).
    foreach(_mqas_ssl_variable IN ITEMS boringssl_INCLUDE_DIR boringssl_ssl_LIBRARY boringssl_crypto_LIBRARY boringssl_ssl_BINARY boringssl_crypto_BINARY)
        unset(${_mqas_ssl_variable})
        unset(${_mqas_ssl_variable} CACHE)
    endforeach()
    find_path(boringssl_INCLUDE_DIR openssl/ssl.h PATHS "${SSL_PATH}/include" NO_DEFAULT_PATH REQUIRED)
    find_library(boringssl_ssl_LIBRARY ssl PATHS "${SSL_PATH}/lib" NO_DEFAULT_PATH REQUIRED)
    find_library(boringssl_crypto_LIBRARY crypto PATHS "${SSL_PATH}/lib" NO_DEFAULT_PATH REQUIRED)
    set(boringssl_LIBRARY
        ${boringssl_ssl_LIBRARY}
        ${boringssl_crypto_LIBRARY}
    )

    find_file(boringssl_ssl_BINARY "ssl${CMAKE_SHARED_LIBRARY_SUFFIX}" PATHS "${SSL_PATH}/bin" NO_DEFAULT_PATH REQUIRED)
    find_file(boringssl_crypto_BINARY "crypto${CMAKE_SHARED_LIBRARY_SUFFIX}" PATHS "${SSL_PATH}/bin" NO_DEFAULT_PATH REQUIRED)
    set(boringssl_BINARY
        ${boringssl_ssl_BINARY}
        ${boringssl_crypto_BINARY}
    )
endif()

if(TRUE)
    include(CheckCXXSourceCompiles)
    set(_mqas_saved_required_includes "${CMAKE_REQUIRED_INCLUDES}")
    set(_mqas_saved_required_libraries "${CMAKE_REQUIRED_LIBRARIES}")
    set(CMAKE_REQUIRED_INCLUDES "${boringssl_INCLUDE_DIR}")
    set(CMAKE_REQUIRED_LIBRARIES "${boringssl_ssl_LIBRARY};${boringssl_crypto_LIBRARY}")
    unset(MQAS_BORINGSSL_HAS_EXTERNAL_PSK CACHE)
    check_cxx_source_compiles("#include <openssl/ssl.h>
        int main() {
            auto *credential = SSL_CREDENTIAL_new_pre_shared_key(nullptr, 0, nullptr, 0, nullptr, nullptr, 0);
            SSL_CREDENTIAL_free(credential);
            return 0;
        }" MQAS_BORINGSSL_HAS_EXTERNAL_PSK)
    set(CMAKE_REQUIRED_INCLUDES "${_mqas_saved_required_includes}")
    set(CMAKE_REQUIRED_LIBRARIES "${_mqas_saved_required_libraries}")
    if(NOT MQAS_BORINGSSL_HAS_EXTERNAL_PSK)
        message(FATAL_ERROR "BoringSSL lacks TLS 1.3 external PSK support. Set SSL_PATH to a matching BoringSSL installation with SSL_CREDENTIAL_new_pre_shared_key.")
    endif()
endif()


message("boringssl_INCLUDE_DIR = ${boringssl_INCLUDE_DIR}")
message("boringssl_LIBRARY = ${boringssl_LIBRARY}")
message("boringssl_BINARY = ${boringssl_BINARY}")
