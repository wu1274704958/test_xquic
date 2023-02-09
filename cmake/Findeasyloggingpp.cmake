find_path(easyloggingpp_INCLUDE_DIR "easylogging++.h" REQUIED)
find_library(easyloggingpp_LIBRARY easyloggingpp REQUIED)

message("easyloggingpp_INCLUDE_DIR = ${easyloggingpp_INCLUDE_DIR}")
message("easyloggingpp_LIBRARY = ${easyloggingpp_LIBRARY}")