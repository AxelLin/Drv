
#include <stdio.h>
#include "ssl_locl.h"


int SSL_set_hello_extension(SSL *s, int ext_type, void *ext_data, int ext_len)
{
	if(s->version >= TLS1_VERSION)
	{
		if(s->tls_extension)
		{
			OPENSSL_free(s->tls_extension);
			s->tls_extension = NULL;
		}

		if(ext_data)
		{
			s->tls_extension = OPENSSL_malloc(sizeof(TLS_EXTENSION) + ext_len);
			if(!s->tls_extension)
			{
				SSLerr(SSL_F_SSL_SET_HELLO_EXTENSION, ERR_R_MALLOC_FAILURE);
				return 0;
			}

			s->tls_extension->type = ext_type;
			s->tls_extension->length = ext_len;
			s->tls_extension->data = s->tls_extension + 1;
			memcpy(s->tls_extension->data, ext_data, ext_len);
		}

		return 1;
	}

	return 0;
}

int SSL_set_hello_extension_cb(SSL *s, int (*cb)(SSL *, TLS_EXTENSION *, void *), void *arg)
{
	if(s->version >= TLS1_VERSION)
	{
		s->tls_extension_cb = cb;
		s->tls_extension_cb_arg = arg;

		return 1;
	}

	return 0;
}
