#protoc --cpp_out=dllexport_decl=MQAS_EXTERN:. common.proto
sed -i '/#include[[:space:]]*"common.pb.h"/c#include "mqas\/tools\/proto\/common.pb.h"' common.pb.cc
sed -i '/#include <string>/i#include "mqas\/macro.h"' common.pb.h
mkdir --parents ../src/tools/proto/
mkdir --parents ../include/mqas/tools/proto/
mv -f common.pb.cc ../src/tools/proto/
mv -f common.pb.h ../include/mqas/tools/proto/
