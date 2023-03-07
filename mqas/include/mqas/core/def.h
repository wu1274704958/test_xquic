#pragma once

namespace mqas::core
{
	enum class InitFlags : uint32_t
	{
		GLOBAL_CLIENT = (1 << 0),
		GLOBAL_SERVER = (1 << 1),
		BOTH = GLOBAL_SERVER | GLOBAL_CLIENT
	};

	enum class EngineFlags : uint32_t
	{
		None = 0,
		Server = (1 << 0),
		HTTP = (1 << 1),
		HTTP_Server = HTTP | Server
	};
    enum class StreamAspect:unsigned char{
        Read = 0,
        Write = 1,
        Both = 2,
        None = 255
    };
    enum class StreamVariantErrcode : uint8_t {
        ok = 0,
        failed_not_find = 1,
    };
}