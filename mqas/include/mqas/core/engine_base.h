#pragma once
#include <mqas/macro.h>
#include <mqas/io/context.h>
#include <toml.hpp>
#include <memory>
namespace mqas::core
{
	class MQAS_EXTERN engine_base
	{
	public:
		engine_base(io::Context&);
		engine_base(engine_base&&) noexcept;
		engine_base(const engine_base&) = delete;
		engine_base& operator=(engine_base&&) = delete;
		engine_base& operator=(const engine_base&) = delete;
		void init(const char* conf_file) noexcept(false);
		void close_socket();
		void close_timer();
		~engine_base();
	protected:
		static std::string load_config(const char* conf_file);
	public:
		io::Context& cxt;
	protected:
		io::UdpSocket* socket_;
		io::Timer* proc_conns_timer_;
		std::unique_ptr<toml::basic_value<toml::discard_comments>> conf_;
	}; 
}
