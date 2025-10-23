message("Find boringssl")
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
    find_path(boringssl_INCLUDE_DIR openssl "${SSL_PATH}/include" REQUIRED)
    find_library(boringssl_ssl_LIBRARY ssl "${SSL_PATH}/lib" REQUIRED)
    find_library(boringssl_crypto_LIBRARY crypto "${SSL_PATH}/lib" REQUIRED)
    set(boringssl_LIBRARY
        ${boringssl_ssl_LIBRARY}
        ${boringssl_crypto_LIBRARY}
    )

    find_file(boringssl_ssl_BINARY "ssl${CMAKE_SHARED_LIBRARY_SUFFIX}" PATHS "${SSL_PATH}/bin" REQUIRED)
    find_file(boringssl_crypto_BINARY "crypto${CMAKE_SHARED_LIBRARY_SUFFIX}" PATHS "${SSL_PATH}/bin" REQUIRED)
    set(boringssl_BINARY
        ${boringssl_ssl_BINARY}
        ${boringssl_crypto_BINARY}
    )
endif()


message("boringssl_INCLUDE_DIR = ${boringssl_INCLUDE_DIR}")
message("boringssl_LIBRARY = ${boringssl_LIBRARY}")
message("boringssl_BINARY = ${boringssl_BINARY}")