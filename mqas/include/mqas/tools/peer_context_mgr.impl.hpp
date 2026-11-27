namespace mqas::tools {

	template<typename T>
	core::peer_context<T>& peer_context_mgr<T>::get_or_create(const sockaddr* addr, T* t)
	{
		return map.try_emplace(io::SocketAddress(*addr), t).first->second;
	}
}

