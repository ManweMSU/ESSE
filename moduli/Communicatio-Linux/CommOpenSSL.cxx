#include "CommOpenSSL.h"
#include "CommUnix.h"
#include "OpenSSLDL.h"
#include <poll.h>

namespace ESSE
{
	namespace Communicatio
	{
		class OpenSSLContextLite : public Object
		{
			oref<OpenSSL::OSSLAPI> _api;
			OpenSSL::SSL_CTX _ssl_ctx;
		public:
			OpenSSLContextLite(OpenSSL::OSSLAPI * api) : _api(api), _ssl_ctx(0) {}
			virtual ~OpenSSLContextLite(void) override { if (_ssl_ctx) _api->SSL_CTX_free(_ssl_ctx); }
			void SetContext(OpenSSL::SSL_CTX ssl_ctx) noexcept { if (_ssl_ctx) _api->SSL_CTX_free(_ssl_ctx); _ssl_ctx = ssl_ctx; if (_ssl_ctx) _api->SSL_CTX_up_ref(_ssl_ctx); }
			OpenSSL::OSSLAPI * GetAPI(void) const noexcept { return _api; }
			OpenSSL::SSL_CTX GetContext(void) noexcept { return _ssl_ctx; }
		};
		class OpenSSLContext : public IOpenSSLEventHandler
		{
			oref<OpenSSL::OSSLAPI> _api;
			OpenSSL::SSL_CTX _ssl_ctx;
			OpenSSL::SSL _ssl;
			OpenSSL::BIO _bio;
			bool _connection_alive, _eos_sent;
			uint _send_eos_mode;
		public:
			OpenSSLContext(OpenSSL::OSSLAPI * api) : _api(api), _ssl_ctx(0), _ssl(0), _bio(0), _connection_alive(true), _send_eos_mode(0), _eos_sent(false) {}
			virtual ~OpenSSLContext(void) override { if (_ssl_ctx) _api->SSL_CTX_free(_ssl_ctx); if (_ssl) _api->SSL_free(_ssl); if (_bio) _api->BIO_free(_bio); }
			virtual OpenSSLEventStatus ProcessConnection(int pollstat, NetworkRequest * req_read, NetworkRequest * req_write, uint pollattr) noexcept override
			{
				_api->ERR_clear_error();
				NetworkRequest * req_process = 0;
				OpenSSLEventStatus request_complete_responce;
				if (req_read && (req_read->in.attributes & REQUEST_ATTRIBUTE_PRIORITY)) {
					req_process = req_read;
					request_complete_responce = OpenSSLEventStatus::CompleteReadRequest;
				} else if (req_write && (req_write->in.attributes & REQUEST_ATTRIBUTE_PRIORITY)) {
					req_process = req_write;
					request_complete_responce = OpenSSLEventStatus::CompleteWriteRequest;
				} else if (req_write && (req_write->in.attributes & pollattr)) {
					req_process = req_write;
					request_complete_responce = OpenSSLEventStatus::CompleteWriteRequest;
				} else if (req_read && (req_read->in.attributes & pollattr)) {
					req_process = req_read;
					request_complete_responce = OpenSSLEventStatus::CompleteReadRequest;
				}
				if (!req_process) return OpenSSLEventStatus::None;
				if (!_connection_alive) {
					if (req_process->out.status) ErrorSet(*req_process->out.status, 0x08, 0x08);
					if (req_process->in.attributes & REQUEST_ATTRIBUTE_ACCEPT_BIT) {
						if (req_process->out.address) (*req_process->out.address).Clear();
						if (req_process->out.channel) (*req_process->out.channel).Clear();
					}
					return request_complete_responce;
				}
				if (pollstat & POLLHUP) {
					if (req_process->out.status) ErrorSet(*req_process->out.status, 0x08, 0x08);
					if (req_process->in.attributes & REQUEST_ATTRIBUTE_ACCEPT_BIT) {
						if (req_process->out.address) (*req_process->out.address).Clear();
						if (req_process->out.channel) (*req_process->out.channel).Clear();
					}
					return request_complete_responce;
				}
				if (req_process->in.attributes & REQUEST_ATTRIBUTE_CONNECT_BIT) {
					auto status = _api->SSL_connect(_ssl);
					if (status <= 0) {
						auto error_status = _api->SSL_get_error(_ssl, status);
						if (error_status == SSL_ERROR_WANT_READ) {
							req_process->in.attributes = REQUEST_ATTRIBUTE_CONNECT_BIT | REQUEST_ATTRIBUTE_POLL_READ | REQUEST_ATTRIBUTE_OPENSSL | REQUEST_ATTRIBUTE_PRIORITY | REQUEST_ATTRIBUTE_INCOMPLETE;
							return OpenSSLEventStatus::PollRenew;
						} else if (error_status == SSL_ERROR_WANT_WRITE) {
							req_process->in.attributes = REQUEST_ATTRIBUTE_CONNECT_BIT | REQUEST_ATTRIBUTE_POLL_WRITE | REQUEST_ATTRIBUTE_OPENSSL | REQUEST_ATTRIBUTE_PRIORITY | REQUEST_ATTRIBUTE_INCOMPLETE;
							return OpenSSLEventStatus::PollRenew;
						} else if (error_status == SSL_ERROR_SYSCALL) {
							if (req_process->out.status) SetPosixError(*req_process->out.status);
							return request_complete_responce;
						} else {
							if (req_process->out.status) ErrorSet(*req_process->out.status, 0x08, 0x0B);
							_connection_alive = false;
							return request_complete_responce;
						}
					} else {
						if (req_process->out.status) ErrorClear(*req_process->out.status);
						return request_complete_responce;
					}
				} else if (req_process->in.attributes & REQUEST_ATTRIBUTE_RECEIVE_BIT) {
					uintptr bytes_read = 0;
					auto status = _api->SSL_read_ex(_ssl, req_process->in.data->GetBuffer() + req_process->in.pointer, req_process->in.length - req_process->in.pointer, &bytes_read);
					if (!status) {
						auto error_status = _api->SSL_get_error(_ssl, status);
						if (error_status == SSL_ERROR_ZERO_RETURN) {
							try {
								req_process->in.data->SetLength(req_process->in.pointer);
								if (req_process->out.data) *req_process->out.data = req_process->in.data;
							} catch (...) {
								if (req_process->out.status) ErrorSet(*req_process->out.status, 2, 0);
								return request_complete_responce;
							}
							if (req_process->out.status) ErrorClear(*req_process->out.status);
							return request_complete_responce;
						} else if (error_status == SSL_ERROR_WANT_READ) {
							req_process->in.attributes = REQUEST_ATTRIBUTE_RECEIVE_BIT | REQUEST_ATTRIBUTE_POLL_READ | REQUEST_ATTRIBUTE_OPENSSL | REQUEST_ATTRIBUTE_INCOMPLETE;
							return OpenSSLEventStatus::PollRenew;
						} else if (error_status == SSL_ERROR_WANT_WRITE) {
							req_process->in.attributes = REQUEST_ATTRIBUTE_RECEIVE_BIT | REQUEST_ATTRIBUTE_POLL_WRITE | REQUEST_ATTRIBUTE_OPENSSL | REQUEST_ATTRIBUTE_INCOMPLETE;
							return OpenSSLEventStatus::PollRenew;
						} else if (error_status == SSL_ERROR_SYSCALL) {
							if (req_process->out.status) SetPosixError(*req_process->out.status);
							return request_complete_responce;
						} else {
							if (req_process->out.status) ErrorSet(*req_process->out.status, 0x08, 0x0B);
							_connection_alive = false;
							return request_complete_responce;
						}
					} else {
						req_process->in.pointer += bytes_read;
						if (req_process->in.pointer == req_process->in.length) {
							if (req_process->out.status) ErrorClear(*req_process->out.status);
							if (req_process->out.data) *req_process->out.data = req_process->in.data;
							return request_complete_responce;
						} else {
							req_process->in.attributes = REQUEST_ATTRIBUTE_RECEIVE_BIT | REQUEST_ATTRIBUTE_POLL_READ | REQUEST_ATTRIBUTE_OPENSSL;
							return OpenSSLEventStatus::PollRenew;
						}
					}
				} else if (req_process->in.attributes & REQUEST_ATTRIBUTE_SEND_BIT) {
					if (_eos_sent) {
						if (req_process->out.status) ErrorSet(*req_process->out.status, 5, 0);
						return request_complete_responce;
					}
					if (req_process->in.data && (!_send_eos_mode || (req_process->in.attributes & REQUEST_ATTRIBUTE_INCOMPLETE))) {
						uintptr written = 0;
						int status;
						if (req_process->in.length == req_process->in.pointer) status = 1;
						else status = _api->SSL_write_ex(_ssl, req_process->in.data->GetBuffer() + req_process->in.pointer, req_process->in.length - req_process->in.pointer, &written);
						if (!status) {
							auto error_status = _api->SSL_get_error(_ssl, status);
							if (error_status == SSL_ERROR_WANT_READ) {
								req_process->in.attributes = REQUEST_ATTRIBUTE_SEND_BIT | REQUEST_ATTRIBUTE_POLL_READ | REQUEST_ATTRIBUTE_OPENSSL | REQUEST_ATTRIBUTE_INCOMPLETE;
								return OpenSSLEventStatus::PollRenew;
							} else if (error_status == SSL_ERROR_WANT_WRITE) {
								req_process->in.attributes = REQUEST_ATTRIBUTE_SEND_BIT | REQUEST_ATTRIBUTE_POLL_WRITE | REQUEST_ATTRIBUTE_OPENSSL | REQUEST_ATTRIBUTE_PRIORITY | REQUEST_ATTRIBUTE_INCOMPLETE;
								return OpenSSLEventStatus::PollRenew;
							} else if (error_status == SSL_ERROR_SYSCALL) {
								if (req_process->out.status) SetPosixError(*req_process->out.status);
								return request_complete_responce;
							} else {
								if (req_process->out.status) ErrorSet(*req_process->out.status, 0x08, 0x0B);
								_connection_alive = false;
								return request_complete_responce;
							}
						} else {
							req_process->in.pointer += written;
							if (req_process->in.pointer == req_process->in.length) {
								if (_send_eos_mode) {
									req_process->in.attributes = REQUEST_ATTRIBUTE_SEND_BIT | REQUEST_ATTRIBUTE_POLL_WRITE | REQUEST_ATTRIBUTE_OPENSSL;
									return OpenSSLEventStatus::PollRenew;
								} else {
									if (req_process->out.status) ErrorClear(*req_process->out.status);
									if (req_process->out.length) *req_process->out.length = req_process->in.length;
									return request_complete_responce;
								}
							} else {
								req_process->in.attributes = REQUEST_ATTRIBUTE_SEND_BIT | REQUEST_ATTRIBUTE_POLL_WRITE | REQUEST_ATTRIBUTE_OPENSSL;
								return OpenSSLEventStatus::PollRenew;
							}
						}
					} else if (!_eos_sent) {
						auto status = _api->SSL_shutdown(_ssl);
						if (status < 0) {
							auto error_status = _api->SSL_get_error(_ssl, status);
							if (error_status == SSL_ERROR_WANT_READ) {
								req_process->in.attributes = REQUEST_ATTRIBUTE_SEND_BIT | REQUEST_ATTRIBUTE_POLL_READ | REQUEST_ATTRIBUTE_OPENSSL;
								return OpenSSLEventStatus::PollRenew;
							} else if (error_status == SSL_ERROR_WANT_WRITE) {
								req_process->in.attributes = REQUEST_ATTRIBUTE_SEND_BIT | REQUEST_ATTRIBUTE_POLL_WRITE | REQUEST_ATTRIBUTE_OPENSSL | REQUEST_ATTRIBUTE_PRIORITY;
								return OpenSSLEventStatus::PollRenew;
							} else if (error_status == SSL_ERROR_SYSCALL) {
								if (req_process->out.status) SetPosixError(*req_process->out.status);
								_connection_alive = false;
								return request_complete_responce;
							} else {
								if (req_process->out.status) ErrorSet(*req_process->out.status, 0x08, 0x0B);
								_connection_alive = false;
								return request_complete_responce;
							}
						} else {
							if (req_process->out.status) ErrorClear(*req_process->out.status);
							_eos_sent = true;
							return _send_eos_mode == 2 ? OpenSSLEventStatus::CloseChannel : OpenSSLEventStatus::CloseChannelWrite;
						}
					} else {
						if (req_process->out.status) ErrorSet(*req_process->out.status, 5, 0);
						return request_complete_responce;
					}
				} else if (req_process->in.attributes & REQUEST_ATTRIBUTE_ACCEPT_BIT) {
					auto status = _api->SSL_accept(_ssl);
					if (status <= 0) {
						auto error_status = _api->SSL_get_error(_ssl, status);
						if (error_status == SSL_ERROR_WANT_READ) {
							req_process->in.attributes = REQUEST_ATTRIBUTE_ACCEPT_BIT | REQUEST_ATTRIBUTE_POLL_READ | REQUEST_ATTRIBUTE_OPENSSL | REQUEST_ATTRIBUTE_PRIORITY | REQUEST_ATTRIBUTE_INCOMPLETE;
							return OpenSSLEventStatus::PollRenew;
						} else if (error_status == SSL_ERROR_WANT_WRITE) {
							req_process->in.attributes = REQUEST_ATTRIBUTE_ACCEPT_BIT | REQUEST_ATTRIBUTE_POLL_WRITE | REQUEST_ATTRIBUTE_OPENSSL | REQUEST_ATTRIBUTE_PRIORITY | REQUEST_ATTRIBUTE_INCOMPLETE;
							return OpenSSLEventStatus::PollRenew;
						} else if (error_status == SSL_ERROR_SYSCALL) {
							if (req_process->out.status) SetPosixError(*req_process->out.status);
							if (req_process->out.address) (*req_process->out.address).Clear();
							if (req_process->out.channel) (*req_process->out.channel).Clear();
							return request_complete_responce;
						} else {
							if (req_process->out.status) ErrorSet(*req_process->out.status, 0x08, 0x0B);
							if (req_process->out.address) (*req_process->out.address).Clear();
							if (req_process->out.channel) (*req_process->out.channel).Clear();
							_connection_alive = false;
							return request_complete_responce;
						}
					} else {
						if (req_process->out.status) ErrorClear(*req_process->out.status);
						return request_complete_responce;
					}
				} else return OpenSSLEventStatus::Failed;
			}
			OpenSSL::OSSLAPI * GetAPI(void) const noexcept { return _api; }
			void InitializeSecurity(NetworkSecurityDesc & sec, ErrorContext & ectx) noexcept
			{
				ESSE_TRY_INTRO
					_ssl_ctx = _api->SSL_CTX_new(_api->TLS_client_method());
					if (!_ssl_ctx) throw OutOfMemoryException();
					if (sec.ignore_security) _api->SSL_CTX_set_verify(_ssl_ctx, SSL_VERIFY_NONE, NULL);
					else _api->SSL_CTX_set_verify(_ssl_ctx, SSL_VERIFY_PEER, NULL);
					if (sec.certificate) {
						auto bio = _api->BIO_new_mem_buf(sec.certificate->GetBuffer(), sec.certificate->GetLength());
						if (!bio) throw OutOfMemoryException();
						auto cert = _api->d2i_X509_bio(bio, 0);
						_api->BIO_free(bio);
						if (!cert) throw InvalidFormatException();
						auto store = _api->X509_STORE_new();
						if (!store) { _api->X509_free(cert); throw OutOfMemoryException(); }
						if (!_api->X509_STORE_add_cert(store, cert)) { _api->X509_free(cert); _api->X509_STORE_free(store); throw OutOfMemoryException(); }
						_api->X509_free(cert);
						auto status1 = _api->SSL_CTX_set1_verify_cert_store(_ssl_ctx, store);
						auto status2 = _api->SSL_CTX_set1_chain_cert_store(_ssl_ctx, store);
						_api->X509_STORE_free(store);
						if (!status1 || !status2) throw InvalidStateException();
					} else {
						if (!_api->SSL_CTX_set_default_verify_paths(_ssl_ctx)) throw InvalidStateException();
					}
					if (!_api->SSL_CTX_set_min_proto_version(_ssl_ctx, TLS1_2_VERSION)) throw InvalidStateException();
					_api->SSL_CTX_set_mode(_ssl_ctx, SSL_MODE_ENABLE_PARTIAL_WRITE);
					_ssl = _api->SSL_new(_ssl_ctx);
					if (!_ssl) throw OutOfMemoryException();
					if (sec.domain.GetLength()) {
						ucs1_string domain = ConvertDomainToPunycode(sec.domain);
						if (!_api->SSL_set_tlsext_host_name(_ssl, domain)) throw OutOfMemoryException();
						if (!_api->SSL_set1_host(_ssl, domain)) throw OutOfMemoryException();
					}
				ESSE_TRY_OUTRO()
			}
			void InitializeChannel(INetworkChannel * channel, ErrorContext & ectx) noexcept
			{
				UnixChannelSetOpenSSLEventHandler(channel, this);
				auto sock = UnixChannelGetSocket(channel);
				ESSE_TRY_INTRO
					if (sock < 0) throw InvalidStateException();
					_bio = _api->BIO_new(_api->BIO_s_fd());
					if (!_bio) throw OutOfMemoryException();
					_api->BIO_set_fd(_bio, sock, BIO_NOCLOSE);
					_api->SSL_set_bio(_ssl, _bio, _bio);
					_bio = 0;
				ESSE_TRY_OUTRO()
			}
			void Initialize(OpenSSLContextLite * context, INetworkChannel * channel, ErrorContext & ectx) noexcept
			{
				UnixChannelSetOpenSSLEventHandler(channel, this);
				auto sock = UnixChannelGetSocket(channel);
				ESSE_TRY_INTRO
					if (sock < 0) throw InvalidStateException();
					_ssl_ctx = context->GetContext();
					_api->SSL_CTX_up_ref(_ssl_ctx);
					_ssl = _api->SSL_new(_ssl_ctx);
					if (!_ssl) throw OutOfMemoryException();
					_bio = _api->BIO_new(_api->BIO_s_fd());
					if (!_bio) throw OutOfMemoryException();
					_api->BIO_set_fd(_bio, sock, BIO_NOCLOSE);
					_api->SSL_set_bio(_ssl, _bio, _bio);
					_bio = 0;
				ESSE_TRY_OUTRO()
			}
			void SendEndOfStream(bool ultimately) noexcept { _send_eos_mode = ultimately ? 2 : 1; }
		};
		class OpenSSLNetworkChannel : public INetworkChannel
		{
			oref<INetworkChannel> _unix;
			oref<OpenSSLContext> _ossl_ctx;
		public:
			OpenSSLNetworkChannel(void) {}
			OpenSSLNetworkChannel(INetworkChannel * base, OpenSSLContext * ossl_ctx) { _unix.SetRetain(base); _ossl_ctx.SetRetain(ossl_ctx); }
			virtual ~OpenSSLNetworkChannel(void) noexcept { if (_unix) { ErrorContext ectx; ErrorClear(ectx); _unix->Close(true, ectx); } }
			virtual string ToStringE(ErrorContext & ectx) const noexcept override { ESSE_TRY_INTRO return U"Open SSL Channel"; ESSE_TRY_OUTRO(string()) }
			virtual void Connect(NetworkAddress * address, ErrorContext * error, IDispatchTask * hdlr, ErrorContext & ectx) noexcept override { ErrorSet(ectx, 1, 0); }
			virtual void Connect(NetworkAddress * address, NetworkSecurityDesc & sec, ErrorContext * error, IDispatchTask * hdlr, ErrorContext & ectx) noexcept 
			{
				ESSE_TRY_INTRO
					if (_unix || _ossl_ctx) throw InvalidStateException();
					auto api = OpenSSL::OSSLAPI::Query();
					_unix = CreateNetworkChannel(address);
					_ossl_ctx = owrap(new OpenSSLContext(api));
					_ossl_ctx->InitializeSecurity(sec, ectx);
					if (ectx.error_code) return;
					oref<IDispatchTask> handler = hdlr;
					auto connection = _unix;
					auto ossl_ctx = _ossl_ctx;
					auto connect_task = CreateStructuredTask<ErrorContext>([error, handler, connection, ossl_ctx](ErrorContext & error_status) {
						if (error_status.error_code) {
							if (error) *error = error_status;
							if (handler) handler->DoTask(0);
						} else {
							ErrorContext ectx;
							ErrorClear(ectx);
							ossl_ctx->InitializeChannel(connection, ectx);
							if (ectx.error_code) {
								if (error) *error = ectx;
								if (handler) handler->DoTask(0);
							} else {
								try {
									NetworkRequest req;
									req.in.attributes = REQUEST_ATTRIBUTE_CONNECT | REQUEST_ATTRIBUTE_OPENSSL | REQUEST_ATTRIBUTE_PRIORITY;
									req.in.length = req.in.pointer = 0;
									req.out.handler = handler;
									req.out.status = error;
									req.out.length = 0;
									req.out.data = 0;
									req.out.address = 0;
									req.out.channel = 0;
									UnixChannelEnqueueRequest(connection, req);
								} catch (...) {
									if (error) ErrorSet(*error, 2, 0);
									if (handler) handler->DoTask(0);
								}
							}
						}
					});
					_unix->Connect(address, &connect_task->Value1, connect_task, ectx);
				ESSE_TRY_OUTRO()
			}
			virtual void Send(DataBlock * data, ErrorContext * error, uintptr * sent, IDispatchTask * hdlr, ErrorContext & ectx) noexcept 
			{
				if (!_unix) { ErrorSet(ectx, 5, 0); return; }
				if (!data || !data->GetLength()) {
					if (error) ErrorClear(*error);
					if (sent) *sent = 0;
					if (hdlr) hdlr->DoTask(0);
					return;
				}
				ESSE_TRY_INTRO
					NetworkRequest req;
					req.in.attributes = REQUEST_ATTRIBUTE_SEND | REQUEST_ATTRIBUTE_OPENSSL;
					req.in.length = data->GetLength();
					req.in.pointer = 0;
					req.in.data.SetRetain(data);
					req.out.handler.SetRetain(hdlr);
					req.out.status = error;
					req.out.length = sent;
					req.out.data = 0;
					req.out.address = 0;
					req.out.channel = 0;
					UnixChannelEnqueueRequest(_unix, req);
				ESSE_TRY_OUTRO()
			}
			virtual void Receive(uintptr length, ErrorContext * error, oref<DataBlock> * data, IDispatchTask * hdlr, ErrorContext & ectx) noexcept 
			{
				if (!_unix) { ErrorSet(ectx, 5, 0); return; }
				if (length < 0) { ErrorSet(ectx, 3, 0); return; }
				ESSE_TRY_INTRO
					auto responce = owrap(new DataBlock(1));
					if (!length) {
						if (error) ErrorClear(*error);
						if (data) *data = responce;
						if (hdlr) hdlr->DoTask(0);
						return;
					}
					responce->SetLength(length);
					NetworkRequest req;
					req.in.attributes = REQUEST_ATTRIBUTE_RECEIVE | REQUEST_ATTRIBUTE_OPENSSL;
					req.in.length = length;
					req.in.pointer = 0;
					req.in.data = responce;
					req.out.handler.SetRetain(hdlr);
					req.out.status = error;
					req.out.length = 0;
					req.out.data = data;
					req.out.address = 0;
					req.out.channel = 0;
					UnixChannelEnterCriticalSection(_unix);
					auto status = _ossl_ctx->ProcessConnection(0, &req, 0, REQUEST_ATTRIBUTE_POLL_READ);
					UnixChannelLeaveCriticalSection(_unix);
					if (status == OpenSSLEventStatus::CompleteReadRequest || status == OpenSSLEventStatus::Failed) {
						if (req.out.handler) req.out.handler->DoTask(0);
					} else UnixChannelEnqueueRequest(_unix, req);
				ESSE_TRY_OUTRO()
			}
			virtual void Close(bool ultimately, ErrorContext & ectx) noexcept 
			{
				if (!_unix) { ErrorSet(ectx, 5, 0); return; }
				ESSE_TRY_INTRO
					UnixChannelEnterCriticalSection(_unix);
					_ossl_ctx->SendEndOfStream(ultimately);
					UnixChannelLeaveCriticalSection(_unix);
					NetworkRequest req;
					req.in.attributes = REQUEST_ATTRIBUTE_SEND | REQUEST_ATTRIBUTE_OPENSSL;
					req.in.length = req.in.pointer = 0;
					req.out.status = 0;
					req.out.length = 0;
					req.out.data = 0;
					req.out.address = 0;
					req.out.channel = 0;
					UnixChannelEnqueueRequest(_unix, req);
				ESSE_TRY_OUTRO()
			}
		};
		class OpenSSLNetworkListener : public INetworkListener
		{
			oref<INetworkListener> _unix;
			oref<OpenSSLContextLite> _ossl_ctx;
		public:
			OpenSSLNetworkListener(void) {}
			virtual ~OpenSSLNetworkListener(void) noexcept { if (_unix) { ErrorContext ectx; ErrorClear(ectx); _unix->Close(ectx); } }
			virtual string ToStringE(ErrorContext & ectx) const noexcept override { ESSE_TRY_INTRO return U"Open SSL Listener"; ESSE_TRY_OUTRO(string()) }
			virtual void Bind(NetworkAddress * address, ErrorContext & ectx) noexcept override { ErrorSet(ectx, 1, 0); }
			virtual void Bind(NetworkAddress * address, NetworkIdentityDesc & idesc, ErrorContext & ectx) noexcept override
			{
				ESSE_TRY_INTRO
					if (_unix || _ossl_ctx) throw InvalidStateException();
					auto api = OpenSSL::OSSLAPI::Query();
					_unix = CreateNetworkListener(address);
					_ossl_ctx = owrap(new OpenSSLContextLite(api));
					if (!idesc.data) { ErrorSet(ectx, 3, 0); return; }
					ucs1_string password = idesc.password;
					auto bio = api->BIO_new_mem_buf(idesc.data->GetBuffer(), idesc.data->GetLength());
					if (!bio) throw OutOfMemoryException();
					auto pkcs12 = api->d2i_PKCS12_bio(bio, 0);
					api->BIO_free(bio);
					if (!pkcs12) throw InvalidFormatException();
					OpenSSL::EVP_PKEY key = 0;
					OpenSSL::X509 cert = 0;
					OpenSSL::STACK_OF_X509 chain = 0;
					auto pkcs12_status = api->PKCS12_parse(pkcs12, password, &key, &cert, &chain);
					api->PKCS12_free(pkcs12);
					if (!pkcs12_status) { ErrorSet(ectx, 0x08, 0x0B); return; }
					auto ssl_ctx = api->SSL_CTX_new(api->TLS_server_method());
					if (!ssl_ctx) { api->EVP_PKEY_free(key); api->X509_free(cert); api->OPENSSL_sk_pop_free(chain, api->X509_free); throw OutOfMemoryException(); }
					_ossl_ctx->SetContext(ssl_ctx);
					api->SSL_CTX_free(ssl_ctx);
					if (!api->SSL_CTX_set_min_proto_version(ssl_ctx, TLS1_2_VERSION)) { api->EVP_PKEY_free(key); api->X509_free(cert); api->OPENSSL_sk_pop_free(chain, api->X509_free); throw OutOfMemoryException(); }
					api->SSL_CTX_set_options(ssl_ctx, SSL_OP_NO_RENEGOTIATION);
					api->SSL_CTX_set_verify(ssl_ctx, SSL_VERIFY_NONE, 0);
					if (!api->SSL_CTX_use_cert_and_key(ssl_ctx, cert, key, chain, 0)) { api->EVP_PKEY_free(key); api->X509_free(cert); api->OPENSSL_sk_pop_free(chain, api->X509_free); throw InvalidStateException(); }
					api->EVP_PKEY_free(key);
					api->X509_free(cert);
					api->OPENSSL_sk_pop_free(chain, api->X509_free);
					_unix->Bind(address, ectx);
				ESSE_TRY_OUTRO()
			}
			virtual void Accept(uintptr limit, ErrorContext * error, oref<INetworkChannel> * channel, oref<NetworkAddress> * address, IDispatchTask * hdlr, ErrorContext & ectx) noexcept override
			{
				ESSE_TRY_INTRO
					if (!_unix || !_ossl_ctx) throw InvalidStateException();
					oref<IDispatchTask> handler = hdlr;
					auto internal_accept = CreateStructuredTask< ErrorContext, oref<INetworkChannel> >([error, channel, address, handler, ossl = _ossl_ctx](const ErrorContext & status, oref<INetworkChannel> & inner) {
						if (status.error_code) {
							if (error) *error = status;
							if (handler) handler->DoTask(0);
						} else {
							oref<OpenSSLContext> full_context;
							oref<OpenSSLNetworkChannel> new_channel;
							try {
								full_context = owrap(new OpenSSLContext(ossl->GetAPI()));
								new_channel = owrap(new OpenSSLNetworkChannel(inner, full_context));
							} catch (...) {
								if (error) ErrorSet(*error, 2, 0);
								if (address) (*address).Clear();
								if (handler) handler->DoTask(0);
								return;
							}
							ErrorContext ectx;
							ErrorClear(ectx);
							full_context->Initialize(ossl, inner, ectx);
							if (ectx.error_code) {
								if (error) *error = ectx;
								if (address) (*address).Clear();
								if (handler) handler->DoTask(0);
								return;
							}
							if (channel) (*channel).SetRetain(new_channel);
							try {
								NetworkRequest req;
								req.in.attributes = REQUEST_ATTRIBUTE_ACCEPT | REQUEST_ATTRIBUTE_OPENSSL | REQUEST_ATTRIBUTE_PRIORITY;
								req.in.length = req.in.pointer = 0;
								req.out.handler.SetRetain(CreateFunctionalTask([handler, new_channel]() { if (handler) handler->DoTask(0); }));
								req.out.status = error;
								req.out.length = 0;
								req.out.data = 0;
								req.out.address = address;
								req.out.channel = channel;
								UnixChannelEnqueueRequest(inner, req);
							} catch (...) {
								if (error) ErrorSet(*error, 2, 0);
								if (address) (*address).Clear();
								if (channel) (*channel).Clear();
								if (handler) handler->DoTask(0);
							}
						}
					});
					_unix->Accept(limit, &internal_accept->Value1, &internal_accept->Value2, address, internal_accept, ectx);
				ESSE_TRY_OUTRO()
			}
			virtual void Close(ErrorContext & ectx) noexcept override { if (!_unix) { ErrorSet(ectx, 5, 0); return; } _unix->Close(ectx); }
		};

		oref<INetworkChannel> CreateNetworkChannelS(NetworkAddress * address) { return oref<INetworkChannel>::CreateOwned(new OpenSSLNetworkChannel); }
		oref<INetworkListener> CreateNetworkListenerS(NetworkAddress * address) { return oref<INetworkListener>::CreateOwned(new OpenSSLNetworkListener); }
	}
}