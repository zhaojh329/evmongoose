#include <assert.h>
#include <string.h>
#include "emn_buf.h"

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

size_t ebuf_insert(struct ebuf *ebuf, size_t off, const void *buf, size_t len)
{
	char *p = NULL;

	assert(ebuf);
	assert(off <= ebuf->len);

	if (ebuf->len + len <= ebuf->size) {
		memmove(ebuf->buf + off + len, ebuf->buf + off, ebuf->len - off);
		if (buf)
			memcpy(ebuf->buf + off, buf, len);
		ebuf->len += len;
	} else {
		size_t new_size = (size_t)((ebuf->len + len) * EBUF_SIZE_MULTIPLIER);
		if ((p = (char *)realloc(ebuf->buf, new_size))) {
			ebuf->buf = p;
			memmove(ebuf->buf + off + len, ebuf->buf + off, ebuf->len - off);
			if (buf)
				memcpy(ebuf->buf + off, buf, len);
			ebuf->len += len;
			ebuf->size = new_size;
		} else {
			len = 0;
		}
	}

	return len;
}

size_t ebuf_append(struct ebuf *ebuf, const void *buf, size_t len)
{
	return ebuf_insert(ebuf, ebuf->len, buf, len);
}

void ebuf_remove(struct ebuf *ebuf, size_t n)
{
	if (n > 0 && n <= ebuf->len) {
		memmove(ebuf->buf, ebuf->buf + n, ebuf->len - n);
		ebuf->len -= n;
	}
}