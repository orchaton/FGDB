#ifndef WAL_H
#define WAL_H

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include "transactions/queue.h"

#TODO: raise flushedLSN when dumping page to disk
#TODO: make single file with recoveryLSNs for each page
#TODO: dump pageLSN with the page

typedef uint64_t lsn_t;

typedef struct {
	lsn_t  LSN;
	page_id_t pid;
	str_t     key;
	srt_t     val;
	enum { INSERT, DELETE, UPDATE } operation:8;
} wal_log_record_t;

typedef struct {
	lsn_t flushedLSN;
	lsn_t LSN;
	int   file;
} wal_logger_t;

wal_logger_t* new_wal_logger(lsn_t LSN, lsn_t fLSN, char* path);

lsn_t write_log(wal_logger_t* w, transaction_t* t);

void update_flushed_LSN(wal_logger_t* w, lsn_t fLSN);

lsn_t give_new_LSN(wal_logger_t* w); 

wal_log_record_t* to_wal_record(wal_logger_t* w, transaction_t* t);

void destroy_wal_record(wal_log_record_t* r);

void destroy_wal_logger(wal_log_logger_t* r);


#endif
