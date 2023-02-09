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
}