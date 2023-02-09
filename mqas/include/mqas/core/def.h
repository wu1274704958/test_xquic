#pragma once

namespace mqas::core
{
	enum class InitFlags : uint32_t
	{
		GLOBAL_CLIENT = (1 << 0),
		GLOBAL_SERVER = (1 << 1),
		BOTH = GLOBAL_SERVER | GLOBAL_CLIENT
	};
}