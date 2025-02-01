#include "rpc.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
/* #include <stdint.h> */


void rpc_encode(FILE* stream, char* content)
{
	fprintf(stream, "Content-Length: %llu\r\n\r\n%s", strlen(content), content);
}


void rpc_decode(FILE* stream, uint32_t* content_len, char** content)
{
	if (fscanf(stream, "Content-Length: %u\r\n\r\n", content_len) != 1)
	{
		*content_len = 0;
		*content = NULL;
	}
	char* temp = malloc(*content_len); // OwO Arena???
	size_t n;
	while(n = fread(temp, 1, *content_len, stream), n < *content_len) // RARE!
	{
		for (int i = n; i >= 0; --i)
		{
			ungetc(temp[i] , stream);
		}
	}
	*content = temp;
}
