#ifndef WORLD_SERVER_H
#define WORLD_SERVER_H

#include "win32.h"
#include "typeDefs.h"
#include "server_enums.h"
#include "jobs.h"
#include "opcode_enum.h"
#include "Stream.h"
#include "area.h"

#include <atomic>

struct buffer {

};

struct worker_thread {
	float					n_p[16];
	uint32					n_t[4];
	Stream					n_s;
	HANDLE					thread;
	job_todo				job_pull[SERVER_JOB_PULL_COUNT];
	uint32					id;
	uint32					out_jobs;
	bool					active;
	Stream					data;
};

typedef void (WINAPI *opcode_fn)(worker_thread*);

struct world_server {
	std::shared_ptr<player> players[SERVER_MAX_PLAYERS];
	area					area;

	HANDLE					iocp;
	SOCKET					socket;
	opcode_fn				opcodes[OP_MAX];

	Stream					data;
	WSABUF					wsa_buf;
	uint8					recv_state;
	e_opcode				opcode;

	std::atomic_bool		workers_run;
	worker_thread			threads[SERVER_MAX_THREAD_COUNT];

	uint32					id;
	uint32					active_threads_count;
	WSADATA					wsa;

} static w_server;

bool WINAPI w_server_init();
static void WINAPI w_server_init_opcodes();

void WINAPI w_server_release();
void WINAPI w_server_close();

DWORD WINAPI w_server_worker(LPVOID);
void WINAPI w_server_process_job(job*, worker_thread*);

std::shared_ptr<player> WINAPI  w_server_get_player(uint32);

void WINAPI w_server_recv(uint8);
void WINAPI w_server_send(Stream*, bool get = true);
static void WINAPI w_server_process_recv(worker_thread*, uint32);

void WINAPI op_init(worker_thread*);
void WINAPI op_close(worker_thread*);
void WINAPI op_player_enter_world(worker_thread*);
void WINAPI op_player_leave_world(worker_thread*);
void WINAPI op_player_move(worker_thread*);
void WINAPI op_get_visible_list(worker_thread*);



#endif
