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
else()
    message("${SSL_PATH}/include")
    find_path(boringssl_INCLUDE_DIR openssl "${SSL_PATH}/include" REQUIED)
    find_library(boringssl_ssl_LIBRARY ssl "${SSL_PATH}/lib" REQUIED)
    find_library(boringssl_crypto_LIBRARY crypto "${SSL_PATH}/lib" REQUIED)
    set(boringssl_LIBRARY
        ${boringssl_ssl_LIBRARY}
        ${boringssl_crypto_LIBRARY}
    )
endif()
message("boringssl_INCLUDE_DIR = ${boringssl_INCLUDE_DIR}")
message("boringssl_LIBRARY = ${boringssl_LIBRARY}")