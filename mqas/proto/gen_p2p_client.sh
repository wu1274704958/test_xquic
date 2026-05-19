#protoc --cpp_out=dllexport_decl=MQAS_EXTERN:. p2p_client.proto
sed -i '/#include[[:space:]]*"p2p_client.pb.h"/c#include "mqas\/tools\/proto\/p2p_client.pb.h"' p2p_client.pb.cc
sed -i '/#include <string>/i#include "mqas\/macro.h"' p2p_client.pb.h
mkdir --parents ../src/tools/proto/
mkdir --parents ../include/mqas/tools/proto/
mv -f p2p_client.pb.cc ../src/tools/proto/
mv -f p2p_client.pb.h ../include/mqas/tools/proto/
