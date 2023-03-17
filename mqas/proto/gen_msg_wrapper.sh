protoc --cpp_out=dllexport_decl=MQAS_EXTERN:. msg_wrapper.proto
sed -i '/#include[[:space:]]*"msg_wrapper.pb.h"/c#include "mqas\/core\/proto\/msg_wrapper.pb.h"' msg_wrapper.pb.cc
sed -i '/#include <string>/i#include "mqas\/macro.h"' msg_wrapper.pb.h
mv -f msg_wrapper.pb.cc ../src/core/proto/
mv -f msg_wrapper.pb.h ../include/mqas/core/proto/
