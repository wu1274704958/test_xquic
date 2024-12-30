namespace mqas::tools {

	template<typename T>
	core::peer_context<T>& peer_context_mgr<T>::get_or_create(const sockaddr* addr, T* t)
	{
		if (map.contains(*addr))
			return map[*addr];
		else{
			map.insert({ *addr, core::peer_context<T>(t) });
			return map[*addr];
		}
	}
}

