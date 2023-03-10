protoc --cpp_out=. msg_wrapper.proto
sed -i '/#include[[:space:]]*"msg_wrapper.pb.h"/c#include "mqas\/core\/proto\/msg_wrapper.pb.h"' msg_wrapper.pb.cc
mv -f msg_wrapper.pb.cc ../../../../src/core/proto/
