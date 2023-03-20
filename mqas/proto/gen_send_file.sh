protoc --cpp_out=dllexport_decl=MQAS_EXTERN:. send_file.proto
sed -i '/#include[[:space:]]*"send_file.pb.h"/c#include "mqas\/tools\/proto\/send_file.pb.h"' send_file.pb.cc
sed -i '/#include <string>/i#include "mqas\/macro.h"' send_file.pb.h
mkdir --parents ../src/tools/proto/
mkdir --parents ../include/mqas/tools/proto/
mv -f send_file.pb.cc ../src/tools/proto/
mv -f send_file.pb.h ../include/mqas/tools/proto/
