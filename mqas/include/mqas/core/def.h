#pragma once
#include <span>
#include <vector>
#include <limits>
#include <optional>
#include <mqas/core/proto/simple.h>
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
        parse_failed,
        not_support,
    };
    template<typename T>
    concept HasStreamTag = requires {
        requires std::is_same_v<typename std::remove_cv<decltype(T::STREAM_TAG)>::type ,size_t>;
    };
    enum class stream_variant_cmd : uint8_t {
        req_use_stream_tag = 1,

    };
    struct MQAS_EXTERN stream_variant_msg{
        stream_variant_cmd cmd;
        uint32_t param1 = 0;    // 4
        uint16_t param2 = 0;    // 2
        uint8_t param3 = 0;     // 1
        StreamVariantErrcode errcode = StreamVariantErrcode::ok;  //1
        std::span<uint8_t> extra_params; // SIZE 1
        [[nodiscard]] std::optional<std::vector<uint8_t>> generate() const;
        static constexpr size_t EXTRA_PARAMS_MAX_SIZE = proto::simple_pkg::PARAMS_MAX_SIZE - 9;
        static std::tuple<std::optional<stream_variant_msg>,size_t> parse_command(const std::span<const uint8_t>& buffer);
    };
}