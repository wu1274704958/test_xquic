#include<mqas/core/engine_base.h>
#include <stdexcept>
#include <mqas/io/udp.h>
#include "easylogging++.h"
#include <lsquic.h>
#include <mqas/io/ip.h>
#include <openssl/pem.h>
#include <mqas/comm/string.h>

#ifdef PF_ANDROID
#endif

int ssl_select_alpn_s(SSL* ssl, const unsigned char** out, unsigned char* outlen,
	const unsigned char* in, unsigned inlen, void* arg)
{
	const auto alpn = static_cast<const char*>(arg);
	//LOG(INFO) << "select alpn";
	std::vector<uint8_t> buf;
	const auto ss = mqas::comm::split(alpn, ';');
	for (auto& a : ss)
	{
		buf.push_back(static_cast<char>(a.size()));
		for (auto c : a)
			buf.push_back(c);
	}
	const int r = SSL_select_next_proto(const_cast<unsigned char**>(out), outlen, in, inlen,
		reinterpret_cast<const uint8_t*>(buf.data()), static_cast<unsigned>(buf.size()));
	if (r == OPENSSL_NPN_NEGOTIATED)
		return SSL_TLSEXT_ERR_OK;
	else {
		const std::string_view in_sv(reinterpret_cast<const char*>(in), inlen);
		LOG(TRACE) << "no supported protocol can be selected from " << in_sv;
		return SSL_TLSEXT_ERR_ALERT_FATAL;
	}
}
