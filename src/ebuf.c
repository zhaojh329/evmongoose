#include "ebuf.h"

void ebuf_init(struct ebuf *ebuf, size_t initial_size)
{
	ebuf->len = ebuf->size = 0;
	ebuf->buf = NULL;
	ebuf_resize(ebuf, initial_size);
}

void ebuf_free(struct ebuf *ebuf)
{
	if (ebuf->buf != NULL) {
		free(ebuf->buf);
		ebuf_init(ebuf, 0);
	}
}

void ebuf_resize(struct ebuf *ebuf, size_t new_size)
{
	if (new_size > ebuf->size || (new_size < ebuf->size && new_size >= ebuf->len)) {
		char *buf = (char *)realloc(ebuf->buf, new_size);
		/*
		* In case realloc fails, there's not much we can do, except keep things as
		* they are. Note that NULL is a valid return value from realloc when
		* size == 0, but that is covered too.
		*/
		if (!buf && new_size > 0)
			return;
		ebuf->buf = buf;
		ebuf->size = new_size;
	}
}