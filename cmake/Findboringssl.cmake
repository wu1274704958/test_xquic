message("Find boringssl")
if(NOT SSL_PATH)
    message("No SSL_PATH find")
    find_path(boringssl_INCLUDE_DIR openssl REQUIED)
    find_library(boringssl_ssl_LIBRARY ssl REQUIED)
    find_library(boringssl_crypto_LIBRARY crypto REQUIED)
    set(boringssl_LIBRARY
        ${boringssl_ssl_LIBRARY}
        ${boringssl_crypto_LIBRARY}
    )

    find_file(boringssl_ssl_BINARY "ssl${CMAKE_SHARED_LIBRARY_SUFFIX}" REQUIED)
    find_file(boringssl_crypto_BINARY "crypto${CMAKE_SHARED_LIBRARY_SUFFIX}" REQUIED)
    set(boringssl_BINARY
        ${boringssl_ssl_BINARY}
        ${boringssl_crypto_BINARY}
    )

else()
    message("${SSL_PATH}/include")
    find_path(boringssl_INCLUDE_DIR openssl "${SSL_PATH}/include" REQUIED)
    find_library(boringssl_ssl_LIBRARY ssl "${SSL_PATH}/lib" REQUIED)
    find_library(boringssl_crypto_LIBRARY crypto "${SSL_PATH}/lib" REQUIED)
    set(boringssl_LIBRARY
        ${boringssl_ssl_LIBRARY}
        ${boringssl_crypto_LIBRARY}
    )

    find_file(boringssl_ssl_BINARY "ssl${CMAKE_SHARED_LIBRARY_SUFFIX}" "${SSL_PATH}/bin" REQUIED)
    find_file(boringssl_crypto_BINARY "crypto${CMAKE_SHARED_LIBRARY_SUFFIX}" "${SSL_PATH}/bin" REQUIED)
    set(boringssl_BINARY
        ${boringssl_ssl_BINARY}
        ${boringssl_crypto_BINARY}
    )
endif()


message("boringssl_INCLUDE_DIR = ${boringssl_INCLUDE_DIR}")
message("boringssl_LIBRARY = ${boringssl_LIBRARY}")
message("boringssl_BINARY = ${boringssl_BINARY}")