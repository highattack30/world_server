#include "world_server.h"

#include "server_enums.h"
#include "utils.h"
#include "config.h"

#include <iostream>

bool WINAPI w_server_init() {

	if (INVALID_SOCKET == (w_server.socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED))) {
		MB_SHOW_ERROR("INVALID_SOCKET!!");
		return false;
	}

	sockaddr_in c_data;
	c_data.sin_addr.S_un.S_addr = config::server.use_all_adapters ? INADDR_ANY : inet_addr(config::server.arbiter_ip);
	c_data.sin_port = config::server.use_default_port ? htons(2017) : htons(config::server.arbiter_port);
	c_data.sin_family = AF_INET;


	if (config::server.reconnect_loop) {
		uint32 attempts = 0;
		while (1) {
			attempts++;
			if (SOCKET_ERROR == connect(w_server.socket, (const sockaddr*)&c_data, sizeof(c_data))) {
				if (WSAECONNREFUSED != WSAGetLastError()) {
					MB_SHOW_ERROR(std::string("WSAConnect -> SOCKET_ERROR ERROR-CODE[" + std::to_string(WSAGetLastError()) + "]").c_str());
					return false;
				}
			}
			else break;

			std::cout << "CONNECTION ATTEMPS:" << attempts << '\r' << std::flush;
			Sleep(0);
		}
	}
	else if (SOCKET_ERROR == connect(w_server.socket, (const sockaddr*)&c_data, sizeof(c_data))) {
		MB_SHOW_ERROR(std::string("WSAConnect -> SOCKET_ERROR ERROR-CODE[" + std::to_string(WSAGetLastError()) + "]").c_str());
		return false;
	}



	printf("\n::CONNECTED::\n\n");

	if (NULL == (w_server.iocp = CreateIoCompletionPort((HANDLE)w_server.socket, NULL, 0, 0))) {
		MB_SHOW_ERROR("CreateIoCompletionPort -->NULL");
		return false;
	}

	a_store(w_server.workers_run, config::server.auto_start ? true : false);
	for (uint32 i = 0; i < config::server.thread_count; i++) {
		w_server.threads[i].id = i;
		w_server.threads[i].thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)w_server_worker, (LPVOID)&w_server.threads[i], 0, &w_server.threads[i].id);
		if (!w_server.threads[i].thread) {
			ZeroMemory(&w_server.threads[i], sizeof(worker_thread));
			printf("FAILED TO OPEN WORKER THREAD %d!\n", i);
			i--;
			continue;
		}
		else
			printf("OPENED WORKER THREAD %d!\n", i);
	}

	return true;
}

void WINAPI w_server_release() {
	for (uint32 i = 0; i < SERVER_MAX_PLAYERS; i++) {
		if (w_server.players[i]) {
			//todo
		}
	}

	w_server_close();

	//todo
}

void WINAPI w_server_close() {
	shutdown(w_server.socket, SD_BOTH);
	closesocket(w_server.socket);
}

DWORD WINAPI w_server_worker(LPVOID arg) {
	worker_thread* this_ = (worker_thread*)arg;

	if (!a_load(w_server.workers_run))
		Sleep(0);

	BOOL result = 0;
	while (1) {
		result = GetQueuedCompletionStatusEx(w_server.iocp, (LPOVERLAPPED_ENTRY)this_->job_pull, SERVER_JOB_PULL_COUNT, &this_->out_jobs, INFINITE, 0);
		if (!result) {
			if (!a_load(w_server.workers_run)) {
				printf("WORKER_THREAD_CLOSED ID[%d]!\n", this_->id); break;
			}
			else if (ERROR_ABANDONED_WAIT_0 == WSAGetLastError()) {
				printf("WORKER_THREAD_CRASHED! WSA-ERROR[%d]\n", WSAGetLastError());
				break;
			}
		}

		for (uint32 i = 0; i < this_->out_jobs; i++) {

			job * j = this_->job_pull[i].job;
			if (j) {

				if (j->type == J_RECV) {
					if (this_->job_pull[i].dwNumberOfBytesTransferred == 0) {
						result = -1;
						break;
					}

					w_server_process_recv((job_recv*)j, this_->job_pull[i].dwNumberOfBytesTransferred, this_);
				}
				else if (j->type == J_SEND) {
					//just delete the job
				}
				else
					w_server_process_job(j, this_);


				delete j;
			}
			else {
				printf("RECV INTERNAL ERROR 1000\n");
			}

			ZeroMemory(&this_->job_pull[i], sizeof(job_todo));
		}

		if (result == -1) {
			printf("RECV_BAD THREAD-ID[%d] WSA-ERROR[%d]\n", this_->id, WSAGetLastError());
			break;
		}
	}

	for (uint32 i = 0; i < SERVER_JOB_PULL_COUNT; i++) {
		if (this_->job_pull[i].job) {
			delete this_->job_pull[i].job;
		}
	}
	ZeroMemory(this_->job_pull, sizeof(job_todo) * SERVER_JOB_PULL_COUNT);
	ZeroMemory(&w_server.threads[this_->id], sizeof(worker_thread));
	return 0;
}

void WINAPI w_server_process_job(job * j, worker_thread * this_) {




	return;
}

void WINAPI w_server_process_recv(job_recv *j, uint32 bytes_transfered, worker_thread* this_) {
	if (!j || !this_) goto return_proc;

	Stream & data = this_->recv_buffer;
	/*RECV_HEAD*/
	if (j->state == 1) {

		uint16 size = data.ReadUInt16();
		this_->opcode = (e_opcode)data.ReadUInt16();

		data.Clear();
		data.Resize(size);
		w_server_recv(this_, 2);
	}
	/*RECV_BODY*/
	else if (j->state == 2) {
		data._pos += (uint16)bytes_transfered;
		if (data._pos < data._size) {
			w_server_recv(this_, 2); //recv rest of packet
			goto return_proc;
		}
			
		if (this_->opcode < OP_MAX) {
			if (w_server.opcodes[this_->opcode])
				w_server.opcodes[this_->opcode](this_);
			else
				printf("OPCODE_NULL[%08x]\n", this_->opcode);
		}
		else
			printf("OPCODE_UNK[%08x]\n", this_->opcode);

		data.Clear();
		data.Resize(4);
		w_server_recv(this_, 1);
	}
	else {
		printf("!!UNK_RECV_STATE [%d]\n", j->state);
	}

return_proc:
	return;
}

void WINAPI w_server_recv(worker_thread *thread, int8 t) {
	thread->wsa_buff.buf = (char*)&thread->recv_buffer._raw[thread->recv_buffer._pos];
	thread->wsa_buff.len = (uint32)(thread->recv_buffer._size - thread->recv_buffer._pos);

	std::unique_ptr<job> j = std::make_unique<job>(new job_recv(t), J_RECV);
	int32 result = WSARecv(w_server.socket, &thread->wsa_buff, 1, NULL, 0, (LPWSAOVERLAPPED)j.get(), NULL);
	if (result == SOCKET_ERROR && (WSA_IO_PENDING != (result = WSAGetLastError()))) {
		printf("RECV_ERROR ID[%d]\n", WSAGetLastError());
	}
	else
		j.release();

	return;
}

void WINAPI w_server_send(Stream *data){
	if (!data) return;
	job_send * j_s = new job_send();
	j_s->data.Write(data->_raw, data->_size);
	j_s->wsa_buff.buf = (char*)j_s->data._raw;
	j_s->wsa_buff.len = (uint32)j_s->data._size;
	std::unique_ptr<job> j = std::make_unique<job>(j_s, J_SEND);

	int32 result = WSASend(w_server.socket, &j_s->wsa_buff, 1, NULL, 0, (LPWSAOVERLAPPED)j.get(), NULL);
	if (result == SOCKET_ERROR && (WSA_IO_PENDING != (result = WSAGetLastError()))) {
		printf("SEND_ERROR ID[%d]\n", WSAGetLastError());
	}
	else
		j.release();

	return;
}
