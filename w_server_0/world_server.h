#ifndef WORLD_SERVER_H
#define WORLD_SERVER_H

#include "win32.h"
#include "typeDefs.h"
#include "server_enums.h"
#include "jobs.h"
#include "opcode_enum.h"

#include <atomic>

struct worker_thread
{
	HANDLE					thread;
	job_todo				job_pull[SERVER_JOB_PULL_COUNT];
	uint32					id;
	uint32					out_jobs;

	Stream					recv_buffer;
	WSABUF					wsa_buff;
	e_opcode				opcode;
};

typedef void (WINAPI *opcode_fn)(worker_thread*);

struct world_server {
	std::shared_ptr<player> players[SERVER_MAX_PLAYERS];
	HANDLE					iocp;
	SOCKET					socket;

	opcode_fn				opcodes[OP_MAX];

	std::atomic_uint32_t	player_count;
	std::atomic_bool		workers_run;
	worker_thread			threads[SERVER_MAX_THREAD_COUNT];
	uint32					active_threads_count;
	WSADATA					wsa;
} static w_server;

bool WINAPI w_server_init();
void WINAPI w_server_release();
void WINAPI w_server_close();

DWORD WINAPI w_server_worker(LPVOID);
void WINAPI w_server_process_job(job*, worker_thread*);
void WINAPI w_server_process_recv(job_recv*, uint32, worker_thread*);
void WINAPI w_server_recv(worker_thread*, int8);
void WINAPI w_server_send(Stream*);
#endif
