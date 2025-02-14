protoc --cpp_out=dllexport_decl=MQAS_EXTERN:. relay.proto
sed -i '/#include[[:space:]]*"relay.pb.h"/c#include "mqas\/tools\/proto\/relay.pb.h"' relay.pb.cc
sed -i '/#include <string>/i#include "mqas\/macro.h"' relay.pb.h
mkdir --parents ../src/tools/proto/
mkdir --parents ../include/mqas/tools/proto/
mv -f relay.pb.cc ../src/tools/proto/
mv -f relay.pb.h ../include/mqas/tools/proto/
