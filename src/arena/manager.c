#include "arena/meta.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "common.h"

// uint16_t * page_tail_bytes_container;

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
	for (size_t i = 0; i < arena->headers->total; i++) {
		destroy_header(arena->headers->items[i]);
	}

	vector_free(arena->headers);
	free(arena->headers);
	return;
}

void destroy_header(page_header_t * header) {
	if (header->state == PAGE_DIRTY) {
		arena_defragmentate_page(header->arena_id, header);
		disk_dump_page(header->page_id, header->arena_id);
	}

	// Destroy reverse keys:
	for (size_t i = 0; i < header->keys->total; i++) {
		free(header->keys->items[i]);
	}

	vector_free(header->keys);
	free(header->keys);
	free(header);
	return;
}

page_header_t * headers_new_page(void) {
	page_header_t * header = (page_header_t *) malloc(sizeof(page_header_t));
	if (!header) {
		return NULL;
	}
	bzero(header, sizeof(*header));

	header->tail_bytes = PAGE_SIZE;
	header->location   = PAGE_INMEMORY;
	header->state      = PAGE_DIRTY;
	header->page_id    = arena->headers->total;
	header->arena_id   = arena_get_next_page();

	header->keys = malloc(sizeof(struct vector));
	vector_init(header->keys, PAGE_HEADER_KEYS_INIT_COUNT);

	vector_add(arena->headers, header);
	return header;
}

page_header_t * headers_alloc_page(size_t value_size) {
	page_header_t * prefered = NULL;
	for (size_t page_id = 0; page_id < arena->headers->total; page_id++) {
		register page_header_t * header;
		if (header->tail_bytes >= value_size) {
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
	assert(prefered->tail_bytes >= value_size);

	return prefered;
}

page_header_key_t * headers_push_key(page_header_t * header, hashmap_key_t * key, off_t page_offset) {
	page_header_key_t * page_key = (page_header_key_t *) malloc(sizeof(page_header_key_t));
	page_key->offset = page_offset;

	vector_add(header->keys, page_key);
	key->header_key_id = header->keys->total - 1; // last one
	return page_key;
}

// Returns offset in page
page_header_t * page_value_set(str_t * value, hashmap_key_t * key) {
	page_header_t * header = headers_alloc_page(value->size);
	arena_page_t  * page   = &arena->pages[ header->arena_id ];

	size_t offset = PAGE_SIZE - header->tail_bytes;
	page_header_key_t * arena_header_key = headers_push_key(header, key, offset);
	key->page   = header->arena_id;

	char * ptr = (char *) page + offset;
	*(typeof(value->size) *) ptr = value->size;
	ptr += sizeof(value->size);
	memcpy(ptr, value->ptr, value->size);

	header->tail_bytes -= value->size;
	// header->lsn = get_lsn();

	return header;
}

page_header_t * page_value_get(hashmap_key_t * key, str_t * retval) {
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

page_header_t * page_value_unset(hashmap_key_t * key, str_t * value) {
	page_header_t * header = arena->headers->items[ key->page ];

	if (header->location == PAGE_INDISK) {
		header->arena_id = heat_page(header->page_id);
		header->location = PAGE_INMEMORY;
	}

	if (header->location == PAGE_INMEMORY) {
		arena_page_t * page = &arena->pages[ header->arena_id ];
		page_header_key_t * header_key = (page_header_key_t *) VECTOR_GET(header->keys[0], page_header_key_t*, key->header_key_id);

		vector_delete(header->keys, key->header_key_id);

		value->size = *(typeof(value->size) *) &page[ header_key->offset ];
		value->ptr  = (char *) &page[ header_key->offset ] + sizeof(typeof(value->size));

		free(header_key);

		header->state = PAGE_DIRTY;

		return header;
	}

	return NULL;
}

static arena_page_id_t heat_page(page_id_t page_id) {
	arena_page_id_t arena_page_id = arena_get_next_page();
	disk_upload_page(disk, page_id, arena_page_id);
	return arena_page_id;
}

// static page_id_t page_value_alloc(size_t bytes) {
// 	assert(bytes < PAGE_SIZE);

// 	for (int32_t page_id = 0; page_id < arena->headers->len; page_id++) {
// 		page_header_t * header = &arena->headers->items[page_id];

// 		if (header->tail_bytes >= bytes) {
// 			if (header->location == PAGE_INMEMORY) {
// 				return page_id;
// 			}

// 			if (header->location == PAGE_INDISK) {
// 				header->arena_id = heat_page(page_id);
// 				header->location = PAGE_INMEMORY;
// 				return page_id;
// 			}
// 		}

// 		if (header->location == PAGE_FREE) {
// 			page_id_t arena_page_id = arena_get_next_page(arena);
// 			disk_new_page(disk);
// 			header->arena_id = arena_page_id;
// 			header->location = PAGE_INMEMORY;

// 			return page_id;
// 		}
// 	}

// 	// Call defragmentator:
// }
