#include "OpenSSLDL.h"
#include <dlfcn.h>

namespace ESSE
{
	namespace OpenSSL
	{
		oref<OSSLAPI> _shared;
		long OSSLAPI::SSL_CTX_set_min_proto_version(SSL_CTX ctx, long ver) { return SSL_CTX_ctrl(ctx, SSL_CTRL_SET_MIN_PROTO_VERSION, ver, 0); }
		long OSSLAPI::SSL_CTX_set_mode(SSL_CTX ctx, long mode) { return SSL_CTX_ctrl(ctx, SSL_CTRL_MODE, mode, 0); }
		long OSSLAPI::SSL_set_tlsext_host_name(SSL ssl, const char * domain) { return SSL_ctrl(ssl, SSL_CTRL_SET_TLSEXT_HOSTNAME, TLSEXT_NAMETYPE_host_name, const_cast<char *>(domain)); }
		long OSSLAPI::BIO_set_fd(BIO bio, int fd, long c) { return BIO_int_ctrl(bio, BIO_C_SET_FD, c, fd); }
		long OSSLAPI::SSL_CTX_set1_verify_cert_store(SSL_CTX ctx, X509_STORE s) { return SSL_CTX_ctrl(ctx, SSL_CTRL_SET_VERIFY_CERT_STORE, 1, s); }
		long OSSLAPI::SSL_CTX_set1_chain_cert_store(SSL_CTX ctx, X509_STORE s) { return SSL_CTX_ctrl(ctx, SSL_CTRL_SET_CHAIN_CERT_STORE, 1, s); }
		OSSLAPI::OSSLAPI(void)
		{
			_library = dlopen("libssl.so", RTLD_NOW);
			if (!_library) throw NotImplementedException();
			try {
				DEFINE_FUNCTION_IMPORT(OPENSSL_init_ssl)
				DEFINE_FUNCTION_IMPORT(ERR_clear_error)
				DEFINE_FUNCTION_IMPORT(ERR_print_errors_cb)
				DEFINE_FUNCTION_IMPORT(TLS_server_method)
				DEFINE_FUNCTION_IMPORT(TLS_client_method)
				DEFINE_FUNCTION_IMPORT(SSL_CTX_new)
				DEFINE_FUNCTION_IMPORT(SSL_CTX_free)
				DEFINE_FUNCTION_IMPORT(SSL_CTX_set_options)
				DEFINE_FUNCTION_IMPORT(SSL_CTX_set_verify)
				DEFINE_FUNCTION_IMPORT(SSL_CTX_use_cert_and_key)
				DEFINE_FUNCTION_IMPORT(SSL_CTX_set_default_verify_paths)
				DEFINE_FUNCTION_IMPORT(SSL_CTX_ctrl)
				DEFINE_FUNCTION_IMPORT(SSL_CTX_up_ref)
				DEFINE_FUNCTION_IMPORT(BIO_s_fd)
				DEFINE_FUNCTION_IMPORT(BIO_new)
				DEFINE_FUNCTION_IMPORT(BIO_new_mem_buf)
				DEFINE_FUNCTION_IMPORT(BIO_free)
				DEFINE_FUNCTION_IMPORT(BIO_int_ctrl)
				DEFINE_FUNCTION_IMPORT(SSL_new)
				DEFINE_FUNCTION_IMPORT(SSL_free)
				DEFINE_FUNCTION_IMPORT(SSL_set1_host)
				DEFINE_FUNCTION_IMPORT(SSL_set_bio)
				DEFINE_FUNCTION_IMPORT(SSL_ctrl)
				DEFINE_FUNCTION_IMPORT(SSL_get_error)
				DEFINE_FUNCTION_IMPORT(SSL_connect)
				DEFINE_FUNCTION_IMPORT(SSL_accept)
				DEFINE_FUNCTION_IMPORT(SSL_read_ex)
				DEFINE_FUNCTION_IMPORT(SSL_write_ex)
				DEFINE_FUNCTION_IMPORT(SSL_shutdown)
				DEFINE_FUNCTION_IMPORT(d2i_X509_bio)
				DEFINE_FUNCTION_IMPORT(X509_free)
				DEFINE_FUNCTION_IMPORT(X509_STORE_new)
				DEFINE_FUNCTION_IMPORT(X509_STORE_free)
				DEFINE_FUNCTION_IMPORT(X509_STORE_add_cert)
				DEFINE_FUNCTION_IMPORT(d2i_PKCS12_bio)
				DEFINE_FUNCTION_IMPORT(PKCS12_free)
				DEFINE_FUNCTION_IMPORT(PKCS12_parse)
				DEFINE_FUNCTION_IMPORT(EVP_PKEY_free)
				DEFINE_FUNCTION_IMPORT(OPENSSL_sk_pop_free)
			} catch (...) { dlclose(_library); throw; }
			OPENSSL_init_ssl(0, 0);
		}
		OSSLAPI::~OSSLAPI(void) { dlclose(_library); }
		oref<OSSLAPI> OSSLAPI::Query(void)
		{
			Memory::AcquireRootLock();
			auto api = _shared;
			Memory::ReleaseRootLock();
			if (!api) {
				api = owrap(new OSSLAPI);
				Memory::AcquireRootLock();
				if (!_shared) _shared = api;
				Memory::ReleaseRootLock();
			}
			return api;
		}
	}
}