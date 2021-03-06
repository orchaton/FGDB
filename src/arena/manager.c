#include "arena/meta.h"
#include "lru/lruq.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "common.h"

static page_id_t page_value_alloc(size_t bytes);
static arena_page_id_t heat_page(page_id_t page_id);

void destroy_header(page_header_t * header);

page_headers_vector_t * init_headers(size_t pages) {
	arena->headers = malloc(sizeof(struct vector));
	vector_init(arena->headers, pages);

	assert(arena->headers);
	return arena->headers;
}

void destroy_headers(void) {

	snapshot();

	for (page_id_t page_id = 0; page_id < arena->headers->total; page_id++) {
		page_header_t * header = VECTOR_GET(arena->headers[0], page_header_t *, page_id);
		destroy_header(header);
	}

	vector_free(arena->headers);
	free(arena->headers);
	return;
}

void snapshot(void) {

	for (page_id_t page_id = 0; page_id < arena->headers->total; page_id++) {
		page_header_t * header = VECTOR_GET(arena->headers[0], page_header_t *, page_id);

		if (header->state == PAGE_DIRTY) {
			arena_defragmentate_page(header->arena_id, header);
			disk_dump_page(header->page_id, header->arena_id);
			header->state = PAGE_CLEAN;
		}
	}
	disk_dump_keys(disk);
}

void destroy_header(page_header_t * header) {
	if (header->state == PAGE_DIRTY) {
		arena_defragmentate_page(header->arena_id, header);
		disk_dump_page(header->page_id, header->arena_id);
	}

	// Destroy reverse keys:
	if (header->keys) {
		for (size_t i = 0; i < header->keys->total; i++) {
			page_header_key_t * key = VECTOR_GET(header->keys[0], page_header_key_t *, i);
			if (key->key_meta_ptr) {
				free(key->key_meta_ptr);
			}
			free(key);
		}

		vector_free(header->keys);
		free(header->keys);
	}
	free(header);
	return;
}

void update_lsn(page_header_t * header, lsn_t LSN) {
	header->pLSN = LSN;
	return;
}

page_header_t * new_header(void) {
	page_header_t * header = (page_header_t *) calloc(1, sizeof(page_header_t));
	return header;
}

page_header_t * headers_new_page(void) {
	page_header_t * header = new_header();

	header->offset_bytes = 0;
	header->location   = PAGE_INMEMORY;
	header->state      = PAGE_DIRTY;
	header->page_id    = arena->headers->total;
	header->arena_id   = arena_get_next_page();
	header->pLSN       = 0;

	header->keys = malloc(sizeof(struct vector));
	vector_init(header->keys, PAGE_HEADER_KEYS_INIT_COUNT);

	vector_add(arena->headers, header);
	lru_touch_page(lru, header);

	return header;
}

page_header_t * headers_alloc_page(size_t value_size) {
	page_header_t * prefered = NULL;
	for (size_t page_id = 0; page_id < arena->headers->total; page_id++) {
		register page_header_t * header = VECTOR_GET(arena->headers[0], page_header_t*, page_id);
		if ((uint16_t) (PAGE_SIZE - header->offset_bytes) >= value_size) {
			return header;
		} else if (
			((PAGE_SIZE - header->fragmentated_bytes) / header->keys->total >= value_size) &&
			(header->fragmentated_bytes / PAGE_SIZE * header->keys->total >= FGDB_FRAGMENTATION_FACTOR)) {
			prefered = header;
		}
	}

	if (!prefered) {
		return headers_new_page();
	}

	arena_defragmentate_page(prefered->arena_id, prefered);

	// We can be more polite here
	// But after defragmentation memory should be enough for new value
	assert(prefered->offset_bytes >= value_size);

	return prefered;
}

page_header_key_t * headers_push_key(page_header_t * header, key_meta_t * key, off_t page_offset) {
	page_header_key_t * page_key = (page_header_key_t *) malloc(sizeof(page_header_key_t));
	page_key->offset = page_offset;

	vector_add(header->keys, page_key);
	key->header_key_id = header->keys->total - 1; // last one
	page_key->key_meta_ptr = key;
	return page_key;
}

// Returns offset in page
page_header_t * page_value_set(str_t * value, key_meta_t * key) {
	page_header_t * header = headers_alloc_page(value->size);
	arena_page_t  * page   = &arena->pages[ header->arena_id ];

	header->location = PAGE_INMEMORY;

	size_t start_from = header->offset_bytes;
	page_header_key_t * arena_header_key = headers_push_key(header, key, start_from);
	key->page   = header->page_id;

	char * ptr = (char *) page + start_from;
	*(typeof(value->size) *) ptr = value->size;
	ptr += sizeof(value->size);
	memcpy(ptr, value->ptr, value->size);

	header->offset_bytes += value->size + sizeof(value->size);
	// header->lsn = get_lsn();

	return header;
}

page_header_t * page_value_get(key_meta_t * key, str_t * retval) {
	page_header_t * header = arena->headers->items[ key->page ];

	if (header->location == PAGE_INDISK) {
		header->arena_id = heat_page(header->page_id);
		header->location = PAGE_INMEMORY;
	}

	if (header->location == PAGE_INMEMORY) {

		arena_page_t * page = (arena_page_t *) arena->pages[ header->arena_id ];
		page_header_key_t * header_key = VECTOR_GET(header->keys[0], page_header_key_t*, key->header_key_id);

		if (NULL == header_key) {
			return NULL;
		}

		char * value = (char *) page + header_key->offset;

		retval->size = *(typeof(retval->size) *) value;
		retval->ptr = value + sizeof(retval->size);

		return header;
	}

	return NULL;
}

page_header_t * page_value_unset(key_meta_t * key, str_t * value) {
	page_header_t * header = arena->headers->items[ key->page ];

	/* Heat page if it's indisk */

	if (header->location == PAGE_INDISK) {
		header->arena_id = heat_page(header->page_id);
		header->location = PAGE_INMEMORY;
	}

	if (header->location == PAGE_INMEMORY) {

		/* Get arena-page */

		arena_page_t * page = &arena->pages[ header->arena_id ];

		/* Take over header_key we deleted */

		page_header_key_t * deleted_header_key = VECTOR_GET(header->keys[0], page_header_key_t*, key->header_key_id);
		vector_delete(header->keys, key->header_key_id);

		/* Recount header_key index for all keys inside this page */

		for (size_t key_id = key->header_key_id; key_id < header->keys->total; key_id++) {
			page_header_key_t * page_header_key = VECTOR_GET(header->keys[0], page_header_key_t*, key_id);
			page_header_key->key_meta_ptr->header_key_id -= 1;
		}

		/* Return value from page */

		char * start_from = (char *) page + deleted_header_key->offset;
		value->size = *(typeof(value->size) *) start_from;
		value->ptr  = start_from + sizeof(value->size);

		free(deleted_header_key);

		/* Mark page as dirty for further defragmentation */

		header->state = PAGE_DIRTY;
		header->fragmentated_bytes += value->size;

		return header;
	}

	return NULL;
}

static arena_page_id_t heat_page(page_id_t page_id) {
	arena_page_id_t arena_page_id = arena_get_next_page();
	disk_upload_page(disk, page_id, arena_page_id);
	page_header_t * header = VECTOR_GET(arena->headers[0], page_header_t*, arena_page_id);
	lru_touch_page(lru, header);
	return arena_page_id;
}
