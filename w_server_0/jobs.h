#ifndef JOBS_H
#define JOBS_H

#include "typeDefs.h"
#include "win32.h"
#include "job_enums.h"
#include "Stream.h"

struct job{
	OVERLAPPED	ov;
	e_job_type	type;
	void*		j_;

	job(void*, e_job_type);
	job();
	~job();
};

struct job_todo
{
	ULONG_PTR	lpCompletionKey;
	job*		job;
	ULONG_PTR	Internal;
	DWORD		dwNumberOfBytesTransferred;

	~job_todo();
	job_todo();
};


struct job_recv {
	job_recv(const int8);
	const uint8		state;
};

struct job_send {
	WSABUF		wsa_buff;
	DWORD		transferBytes;
	DWORD		flags;
	Stream		data;
};

#endif
