# Building BoringSSL for TLS 1.3 external PSK

mqas requires a BoringSSL build exposing `SSL_CREDENTIAL_new_pre_shared_key`. The vcpkg 2024-09-13 port is too old. The helper script pins BoringSSL revision `0131ae9d78e42fcc2c1811315e62410ad96d8230` and builds a separate shared-library installation without modifying the system vcpkg tree.

On Windows, run these commands from a Visual Studio x64 developer prompt with CMake, Ninja, NASM, Git and Go available:

```powershell
cmake -DMQAS_BORINGSSL_WORK_DIR=D:/code/deps/mqas-boringssl -P cmake/BuildBoringSSL.cmake
cmake -S . -B build -DSSL_PATH=D:/code/deps/mqas-boringssl/install
```

Run the first command from the mqas repository root. For the gsync parent configure with the same `-DSSL_PATH`, or set `MQAS_BORINGSSL_ROOT` to that installation. The selected header, import libraries and runtime libraries must come from the same build. The configure-time check fails if the external PSK API is absent.

A P2P peer uses a per-session TLS 1.3 external PSK issued by the helper over the pinned lobby connection. The peer connection callback is delivered after the authenticated handshake. Old helper requests without protocol version 1 are rejected.

To run the isolated TLS and QUIC external-PSK checks, configure with `-DMQAS_BUILD_PSK_TESTS=ON`, build `mqas_boringssl_external_psk` and `mqas_quic_external_psk`, then run CTest with `-R mqas_.*external_psk`. These checks cover the TLS and QUIC handshake; run the gsync P2P peer tests as well for the complete lobby-to-datagram path.

Compatibility (Windows x64): the upgraded library exposed two lsquic defects, fixed in the pinned lsquic submodule revision: switching compatible versions after sending an Initial and encoding a QUIC v2 Retry with v1 packet-type bits. The isolated QUIC tests cover stepped packet processing and forced Retry; the gsync lobby/P2P test also passes with default version negotiation. The patched lsquic revision must be available from a remote accessible to anyone cloning mqas.

For an mqas `sub_engine` client, `[engine_config] server_cert_sha256` may contain the 64 hexadecimal digits of the SHA-256 fingerprint of the expected server leaf certificate. When configured, the TLS handshake fails unless the received leaf certificate matches the pin. Provision the expected fingerprint through a trusted channel. An omitted pin retains legacy behavior for ordinary connections and does **not** authenticate the server. P2P helper requests and credentials require a successfully verified, pinned lobby connection. The P2P engine uses a separate configuration without `server_cert_sha256`: its server authentication is the external PSK. Session credentials expire after two minutes, so participating clocks must be synchronized. Lobby peer IDs identify registered sessions; this mechanism does not authenticate an account or prevent an untrusted party from registering a name.

## Connection security state

The sub-engine owns its immutable TLS configuration and LSQUIC callback context. `IConnect::security_info()` returns an atomic snapshot for that connection: pending/succeeded/failed and, only after a successful handshake, the peer authentication method. A pinned certificate policy and an external-PSK-only policy are configured internally; application streams must not treat an engine setting as a completed handshake. On the current LSQUIC server path a connection is published after the mini connection completes its handshake, whereas clients receive `on_hsk_done`. Both paths populate connection security before the P2P layer announces the connection.

A gsync helper result is bound internally to its socket, both peer IDs, selected role and remote address. `GSY_StartP2PPeer` validates the supplied values against that negotiation and acquires the socket and session in a single locked snapshot. Closing the lobby can remove the handle while an already-started peer keeps its own socket and session references.
