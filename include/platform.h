#include <asio.hpp>

inline void platform_handle(asio::ip::udp::socket& s)
{
#ifdef _MSC_VER
    auto sock = s.native_handle();

    BOOL bEnalbeConnRestError = FALSE;
    DWORD dwBytesReturned = 0;
    int ret = WSAIoctl(sock, SIO_UDP_CONNRESET, &bEnalbeConnRestError, sizeof(bEnalbeConnRestError),
        NULL, 0, &dwBytesReturned, NULL, NULL);

    if(ret == SOCKET_ERROR){
        int error = WSAGetLastError();
        printf("platform_handle err = %d\n", error);
    }
        
#endif
}