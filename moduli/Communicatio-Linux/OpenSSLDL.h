#pragma once

#include <Cor/Classes/CorObject.h>
#include <Cor/IO/CorDL.h>

namespace ESSE
{
	namespace OpenSSL
	{
		#define DEFINE_HANDLE_TYPE(NAME) typedef struct __internal_##NAME * NAME;
		#define DEFINE_FUNCTION_POINTER(NAME, RV, ARGS) typedef RV (* func_##NAME)ARGS noexcept; func_##NAME NAME;
		#define DEFINE_FUNCTION_IMPORT(NAME) NAME = reinterpret_cast<func_##NAME>(ESSE::IO::GetLibraryRoutine(_library, #NAME)); if (!NAME) throw NotImplementedException();

		DEFINE_HANDLE_TYPE(SSL_CTX)
		DEFINE_HANDLE_TYPE(SSL)
		DEFINE_HANDLE_TYPE(SSL_METHOD)
		DEFINE_HANDLE_TYPE(BIO)
		DEFINE_HANDLE_TYPE(BIO_METHOD)
		DEFINE_HANDLE_TYPE(X509)
		DEFINE_HANDLE_TYPE(X509_STORE)
		DEFINE_HANDLE_TYPE(STACK_OF_X509)
		DEFINE_HANDLE_TYPE(EVP_PKEY)
		DEFINE_HANDLE_TYPE(PKCS12)

		#define SSL_CTRL_SET_MIN_PROTO_VERSION 123
		#define SSL_CTRL_MODE 33
		#define SSL_CTRL_SET_TLSEXT_HOSTNAME 55
		#define SSL_CTRL_SET_VERIFY_CERT_STORE 106
		#define SSL_CTRL_SET_CHAIN_CERT_STORE 107
		#define TLSEXT_NAMETYPE_host_name 0
		#define BIO_C_SET_FD 104
		#define SSL3_VERSION 0x0300
		#define TLS1_VERSION 0x0301
		#define TLS1_1_VERSION 0x0302
		#define TLS1_2_VERSION 0x0303
		#define TLS1_3_VERSION 0x0304
		#define SSL_MODE_ENABLE_PARTIAL_WRITE 0x00000001U
		#define SSL_ERROR_NONE 0
		#define SSL_ERROR_SSL 1
		#define SSL_ERROR_WANT_READ 2
		#define SSL_ERROR_WANT_WRITE 3
		#define SSL_ERROR_WANT_X509_LOOKUP 4
		#define SSL_ERROR_SYSCALL 5
		#define SSL_ERROR_ZERO_RETURN 6
		#define SSL_VERIFY_NONE 0x00
		#define SSL_VERIFY_PEER 0x01
		#define BIO_NOCLOSE 0x00
		#define BIO_CLOSE 0x01
		#define SSL_OP_NO_RENEGOTIATION (1 << 30)

		class OSSLAPI : public Object
		{
			handle _library;
		public:
			DEFINE_FUNCTION_POINTER(OPENSSL_init_ssl, int, (uint64, const void *))
			DEFINE_FUNCTION_POINTER(ERR_clear_error, void, (void))
			DEFINE_FUNCTION_POINTER(ERR_print_errors_cb, void, (int (*) (const char *, uintptr, void *), void *))
			DEFINE_FUNCTION_POINTER(TLS_server_method, SSL_METHOD, (void))
			DEFINE_FUNCTION_POINTER(TLS_client_method, SSL_METHOD, (void))
			DEFINE_FUNCTION_POINTER(SSL_CTX_new, SSL_CTX, (SSL_METHOD))
			DEFINE_FUNCTION_POINTER(SSL_CTX_free, void, (SSL_CTX))
			DEFINE_FUNCTION_POINTER(SSL_CTX_set_options, uint64, (SSL_CTX, uint64))
			DEFINE_FUNCTION_POINTER(SSL_CTX_set_verify, void, (SSL_CTX, int, const void *))
			DEFINE_FUNCTION_POINTER(SSL_CTX_use_cert_and_key, int, (SSL_CTX, X509, EVP_PKEY, STACK_OF_X509, int))
			DEFINE_FUNCTION_POINTER(SSL_CTX_set_default_verify_paths, int, (SSL_CTX))
			DEFINE_FUNCTION_POINTER(SSL_CTX_ctrl, long, (SSL_CTX, int, long, void *))
			DEFINE_FUNCTION_POINTER(SSL_CTX_up_ref, int, (SSL_CTX))
			DEFINE_FUNCTION_POINTER(BIO_s_fd, BIO_METHOD, (void))
			DEFINE_FUNCTION_POINTER(BIO_new, BIO, (BIO_METHOD))
			DEFINE_FUNCTION_POINTER(BIO_new_mem_buf, BIO, (const void *, int))
			DEFINE_FUNCTION_POINTER(BIO_free, int, (BIO))
			DEFINE_FUNCTION_POINTER(BIO_int_ctrl, long, (BIO, int, long, int))
			DEFINE_FUNCTION_POINTER(SSL_new, SSL, (SSL_CTX))
			DEFINE_FUNCTION_POINTER(SSL_free, void, (SSL))
			DEFINE_FUNCTION_POINTER(SSL_set1_host, int, (SSL, const char *))
			DEFINE_FUNCTION_POINTER(SSL_set_bio, void, (SSL, BIO, BIO))
			DEFINE_FUNCTION_POINTER(SSL_ctrl, long, (SSL, int, long, void *))
			DEFINE_FUNCTION_POINTER(SSL_get_error, int, (SSL, int))
			DEFINE_FUNCTION_POINTER(SSL_connect, int, (SSL))
			DEFINE_FUNCTION_POINTER(SSL_accept, int, (SSL))
			DEFINE_FUNCTION_POINTER(SSL_read_ex, int, (SSL, void *, uintptr, uintptr *))
			DEFINE_FUNCTION_POINTER(SSL_write_ex, int, (SSL, const void *, uintptr, uintptr *))
			DEFINE_FUNCTION_POINTER(SSL_shutdown, int, (SSL))
			DEFINE_FUNCTION_POINTER(d2i_X509_bio, X509, (BIO, X509 *))
			DEFINE_FUNCTION_POINTER(X509_free, void, (X509))
			DEFINE_FUNCTION_POINTER(X509_STORE_new, X509_STORE, (void))
			DEFINE_FUNCTION_POINTER(X509_STORE_free, void, (X509_STORE))
			DEFINE_FUNCTION_POINTER(X509_STORE_add_cert, int, (X509_STORE, X509))
			DEFINE_FUNCTION_POINTER(d2i_PKCS12_bio, PKCS12, (BIO, PKCS12 *))
			DEFINE_FUNCTION_POINTER(PKCS12_free, void, (PKCS12))
			DEFINE_FUNCTION_POINTER(PKCS12_parse, int, (PKCS12, const char *, EVP_PKEY *, X509 *, STACK_OF_X509 *))
			DEFINE_FUNCTION_POINTER(EVP_PKEY_free, void, (EVP_PKEY))
			DEFINE_FUNCTION_POINTER(OPENSSL_sk_pop_free, void, (STACK_OF_X509, func_X509_free))
			long SSL_CTX_set_min_proto_version(SSL_CTX, long);
			long SSL_CTX_set_mode(SSL_CTX, long);
			long SSL_set_tlsext_host_name(SSL, const char *);
			long BIO_set_fd(BIO, int, long);
			long SSL_CTX_set1_verify_cert_store(SSL_CTX, X509_STORE);
			long SSL_CTX_set1_chain_cert_store(SSL_CTX, X509_STORE);
		public:
			OSSLAPI(void);
			virtual ~OSSLAPI(void) override;
			static oref<OSSLAPI> Query(void);
		};
	}
}