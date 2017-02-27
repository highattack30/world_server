#include "world_server.h"

#include "server_enums.h"
#include "utils.h"
#include "player.h"
#include "config.h"

#include <iostream>

bool WINAPI w_server_init() {
	if (!load_area_from_file(config::server.area_file.c_str(), w_server.area)) {
		printf("FAILED TO LOAD AREA FILE!!\n");
		return false;
	}

	if (NULL == (w_server.iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0))) {
		MB_SHOW_ERROR("CreateIoCompletionPort -->NULL");
		return false;
	}

	a_store(w_server.workers_run, true);
	SYSTEM_INFO s_info;
	GetSystemInfo(&s_info);
	uint32 t_count = 2;// (s_info.dwNumberOfProcessors * 2) - config::server.active_thread_count;
	for (uint32 i = 0; i < t_count; i++) {
		w_server.threads[i].id = i;
		w_server.threads[i].thread = CreateThread(NULL, 0, w_server_worker, (LPVOID)&w_server.threads[i], 0, 0);
		if (!w_server.threads[i].thread) {
			ZeroMemory(&w_server.threads[i], sizeof(worker_thread));
			printf("FAILED TO OPEN WORKER THREAD %d!\n", i);
			i--;
			continue;
		}
		else
			printf("OPENED WORKER THREAD %d!\n", i);
	}

	w_server_init_opcodes();
	if (INVALID_SOCKET == (w_server.socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED))) {
		MB_SHOW_ERROR("INVALID_SOCKET!!");
		return false;
	}

	if (w_server.iocp != CreateIoCompletionPort((HANDLE)w_server.socket, w_server.iocp, (ULONG_PTR)0, 0)) {
		MB_SHOW_ERROR("CreateIoCompletionPort((HANDLE)w_server.socket,...) -->NULL WSA-ERROR[%d]\n", WSAGetLastError());
		return false;
	}


	sockaddr_in sock_data;
	sock_data.sin_addr.S_un.S_addr = inet_addr(config::server.arbiter_ip);
	sock_data.sin_port = htons(config::server.arbiter_port);
	sock_data.sin_family = AF_INET;

	uint32 count = 0;
	while (SOCKET_ERROR == WSAConnect(w_server.socket, (const sockaddr*)&sock_data, sizeof(sock_data), NULL, NULL, NULL, NULL))
	{
		count++;
		int err = WSAGetLastError();

		if (err == 10035) {
			printf("CONNECTION_FAILED ATTEAMPT[%d], REATTEMPTING IN 1500ms...\n", count);
			Sleep(1500);
		}
		else if (err == 10056) {
			break;
		}
		else if (err == 10061) {
			printf("CONNECTION FAILED, ARBITER_SERVER_IS_DONW!\n");
			return false;
		}
		else {
			printf("CONNECTION FAILED, WSA-ERROR[%d].\n", err);
			return false;
		}
		if (count >= 4) {
			printf("CONNECTION FAILED, ARBITER_SERVER UNREACHEABLE.\n");
			return false;
		}

	}
	printf("CONNECTED TO ARBITER_SERVER!\n");


	Stream data;
	data.Resize(12);
	data.WriteUInt16(0);
	data.WriteUInt16(C_W_INIT);
	data.WriteUInt32(config::server.area_id);
	data.WriteUInt32(config::server.continent_id);
	data.WriteUInt32(config::server.channel_id);
	data.WriteString(config::server.name);
	data.WritePos(0);
	w_server_send(&data);
	printf("SENT_INIT_DATA\n");

	w_server.data.Resize(4);
	w_server_recv(0);


	printf("\n::WORLD_NODE ACTIVE::\n\n");
	return true;
}

void WINAPI w_server_init_opcodes() {
	w_server.opcodes[S_W_INIT] = op_init;
	w_server.opcodes[S_W_CLOSE] = op_close;
	w_server.opcodes[S_W_PLAYER_ENTER_WORLD] = op_player_enter_world;
	w_server.opcodes[S_W_PLAYER_LEAVE_WORLD] = op_player_leave_world;
	w_server.opcodes[S_W_PLAYER_MOVE] = op_player_move;
	w_server.opcodes[S_W_GET_VISIBLE_PLAYERS] = op_get_visible_list;

	return;
}

void WINAPI w_server_release() {
	a_store(w_server.workers_run, false);

	for (uint32 i = 0; i < SERVER_MAX_PLAYERS; i++) {
		if (w_server.players[i]) {
			//todo
		}
	}

	CloseHandle(w_server.iocp);
	PostQueuedCompletionStatus(w_server.iocp, 0, 0, 0);
	for (uint32 i = 0; i < SERVER_MAX_THREAD_COUNT; i++) {
		if (w_server.threads[i].active) {
			WaitForSingleObject(w_server.threads[i].thread, INFINITE);
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
	this_->active = true;
	if (!a_load(w_server.workers_run))
		Sleep(0);

	BOOL result = 0;
	while (1) {
		result = GetQueuedCompletionStatusEx(w_server.iocp, (LPOVERLAPPED_ENTRY)this_->job_pull, SERVER_JOB_PULL_COUNT, &this_->out_jobs, INFINITE, 0);

		if (!a_load(w_server.workers_run)) {
			break;
		}

		if (!result) {
			if (ERROR_ABANDONED_WAIT_0 == WSAGetLastError()) {
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

					w_server_process_recv(this_, (uint32)this_->job_pull[i].dwNumberOfBytesTransferred);
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

	printf("WORKER_THREAD_CLOSED ID[%d]!\n", this_->id);
	this_->active = false;
	return 0;
}

void WINAPI w_server_process_job(job * j, worker_thread * this_) {




	return;
}

std::shared_ptr<player> WINAPI w_server_get_player(uint32 id) {
	if (id < SERVER_MAX_PLAYERS)
		return w_server.players[id];
	return nullptr;
}

void WINAPI w_server_recv(uint8 recv_state) {
	w_server.recv_state = recv_state;
	w_server.wsa_buf.buf = (char*)&w_server.data._raw[w_server.data._pos];
	w_server.wsa_buf.len = w_server.data._size - w_server.data._pos;

	DWORD flags = 0;
	std::unique_ptr<job> j = std::make_unique<job>((void*)nullptr, J_RECV);
	int32 result = WSARecv(w_server.socket, &w_server.wsa_buf, 1, NULL, &flags, (LPWSAOVERLAPPED)j.get(), NULL);
	if (result == SOCKET_ERROR && (WSA_IO_PENDING != WSAGetLastError())) {
		printf("RECV_ERROR ID[%d]\n", WSAGetLastError());
	}
	else
		j.release();



	if (w_server.wsa_buf.len > 500) {
		printf("POSTED RECV SIZE[%d]\n", w_server.wsa_buf.len);
	}

	return;
}

void WINAPI w_server_send(Stream *data, bool get) {
	job_send * j_s = new job_send();
	if (get) {
		j_s->data._raw = data->_raw;
		j_s->data._size = data->_size;
		data->Free();
	}
	else {
		j_s->data.Write(data->_raw, data->_size);
	}

	j_s->wsa_buff.buf = (char*)j_s->data._raw;
	j_s->wsa_buff.len = j_s->data._size;
	std::unique_ptr<job> j = std::make_unique<job>(j_s, J_SEND);

	int32 result = WSASend(w_server.socket, &j_s->wsa_buff, 1, NULL, 0, (LPWSAOVERLAPPED)j.get(), NULL);
	if (result == SOCKET_ERROR && (WSA_IO_PENDING != (result = WSAGetLastError()))) {
		printf("SEND_ERROR ID[%d]\n", WSAGetLastError());
	}
	else
		j.release();

	return;
}

void WINAPI w_server_process_recv(worker_thread* t, uint32 recv_bytes) {
	if (!w_server.recv_state) {

		uint16 size = w_server.data.ReadUInt16();
		w_server.opcode = (e_opcode)w_server.data.ReadUInt16();

		if (size == 4) {
			e_opcode opcode = w_server.opcode;
			w_server_recv(0);




			if (opcode > OP_MAX) {
				printf("UNK OPCODE[%hu]\n", opcode);
				w_server_recv(0);
				return;
			}
			else if (!w_server.opcodes[opcode]) {
				printf("NULL OPCODE[%hu]\n", opcode);
				w_server_recv(0);
				return;
			}

			w_server.opcodes[opcode](t);
		}
		else {
			w_server.data.Clear();
			w_server.data.Resize(size - 4);
			w_server_recv(1);
		}
	}
	else {
		w_server.data._pos += recv_bytes;
		if (w_server.data._size > w_server.data._pos) {
			w_server_recv(1);
			return;
		}

		e_opcode opcode = w_server.opcode;
		if (opcode > OP_MAX) {
			printf("UNK OPCODE[%hu]\n", opcode);
			w_server_recv(0);
			return;
		}
		else if (!w_server.opcodes[opcode]) {
			printf("NULL OPCODE[%hu]\n", opcode);
			w_server_recv(0);
			return;
		}

		t->data.Clear();
		t->data.Resize(w_server.data._size);
		t->data.Write(w_server.data._raw, t->data._size);
		t->data._pos = 0;

		w_server.data.Clear();
		w_server.data.Resize(4);
		w_server_recv(0);


		w_server.opcodes[opcode](t);
	}
	return;
}


void WINAPI op_init(worker_thread * t) {
	uint32 rejected = t->data.ReadUInt8();
	if (rejected) {
		w_server_close();
		printf("WORLD_SERVER_REJECTED [%s]\n", rejected == 1 ? "AREA_REPLICATION" : "CHANNEL_REPLICATION");
	}
	else {
		uint32 node_id = t->data.ReadUInt32();
		w_server.id = node_id;

		printf("WORLD_SERVER_ACCEPTED NODE_ID[%d]\n", node_id);
		SetConsoleTitle(config::server.name.c_str());
	}
	return;
}

void WINAPI op_close(worker_thread *t) {
	w_server_close();
	w_server.area.clear();

	printf("ARBITER_SERVER_CLOSE_REQUEST! CLOSED_SOCKET, CLEARED_AREA!\n");
	return;
}

void WINAPI op_player_enter_world(worker_thread * t) {
	uint32 id = t->data.ReadUInt32();
	if (w_server.players[id])
	{
		//todo send fail
		return;
	}
	uint8 result = 1;
	w_server.players[id] = std::make_shared<player>(id);
	a_store(w_server.players[id]->x, t->data.ReadFloat());
	a_store(w_server.players[id]->y, t->data.ReadFloat());
	a_store(w_server.players[id]->z, t->data.ReadFloat());
	a_store(w_server.players[id]->w, t->data.ReadInt16());

	//todo recv other data

	t->n_p[0] = w_server.players[id]->x - SERVER_VISIBLE_SPAWN_RANGE;
	t->n_p[1] = w_server.players[id]->y - SERVER_VISIBLE_SPAWN_RANGE;
	t->n_p[2] = 2 * SERVER_VISIBLE_SPAWN_RANGE;
	if (!w_server.area.put_player(w_server.players[id], t->n_p)) {
		printf("FAILED TO ADD PLAYER TO WORLD CELLS/SECTION..\n");
		result = 0x00;
	}

	w_server.area.init_player(w_server.players[id], t->n_p);

	Stream data;
	data.Resize(17);
	data.WriteUInt16(17);
	data.WriteUInt16(C_W_PLAYER_ENTER_WORLD);
	data.WriteUInt32(w_server.id);
	data.WriteUInt8(result);
	data.WriteUInt32(id);
	data.WriteUInt32(w_server.players[id]->get_count());
	w_server.players[id]->write_to_stream(data);
	data.WritePos(0);

	w_server_send(&data);
	return;
}

void WINAPI op_player_leave_world(worker_thread * t) {
	uint32 id = t->data.ReadUInt32();
	uint32 next_id = t->data.ReadUInt32();
	uint16  op = t->data.ReadUInt16();

	Stream data;
	data.Resize(20);
	data.WriteUInt16(0);
	data.WriteUInt16(C_W_PLAYER_LEAVE_WORLD);
	data.WriteUInt32(id);
	data.WriteUInt16(op);

	data.WriteFloat(a_load(w_server.players[id]->x));
	data.WriteFloat(a_load(w_server.players[id]->y));
	data.WriteFloat(a_load(w_server.players[id]->z));
	data.WriteInt16(a_load(w_server.players[id]->w));
	w_server.players[id]->collect(data);
	data.WritePos(0);
	w_server_send(&data);

	w_server.area.remove_player(w_server.players[id]);
	w_server.players[id]->clear();
	w_server.players[id] = nullptr;
	return;
}

void WINAPI op_player_move(worker_thread * t) {
	uint32 id = t->data.ReadUInt32();
	std::shared_ptr<player> p = w_server.players[id];
	if (!p) return;


	t->n_t[0] = t->data.ReadUInt16(); //heading
	t->n_t[1] = t->data.ReadUInt16(); //type
	t->n_t[2] = t->data.ReadUInt16(); //speed
	t->n_t[3] = t->data.ReadUInt16(); //time


	t->n_p[0] = t->data.ReadFloat();	 //new x	  //new spawn square
	t->n_p[1] = t->data.ReadFloat();	 //new y	  //new spawn square
	t->n_p[2] = 2 * SERVER_VISIBLE_SPAWN_RANGE;		  //new spawn square

	t->n_p[12] = min(t->n_p[0], a_load(p->x)) - SERVER_VISIBLE_DESPAWN_RANGE;
	t->n_p[13] = min(t->n_p[1], a_load(p->y)) - SERVER_VISIBLE_DESPAWN_RANGE;
	t->n_p[14] = 2 * SERVER_VISIBLE_DESPAWN_RANGE - abs(t->n_p[0] - a_load(p->x));
	t->n_p[15] = 2 * SERVER_VISIBLE_DESPAWN_RANGE - abs(t->n_p[1] - a_load(p->y));
	

	t->n_p[3] = t->n_p[0] - SERVER_VISIBLE_DESPAWN_RANGE;//new despawn square
	t->n_p[4] = t->n_p[1] - SERVER_VISIBLE_DESPAWN_RANGE;//new despawn square
	t->n_p[5] = 2 * SERVER_VISIBLE_DESPAWN_RANGE;		 //new despawn square

	a_store(w_server.players[id]->x, t->n_p[0]);
	a_store(w_server.players[id]->y, t->n_p[1]);
	a_store(w_server.players[id]->z, t->data.ReadFloat());  //new z
	a_store(w_server.players[id]->w, t->n_t[0]);

	t->n_p[9] = t->data.ReadFloat();	 //target x
	t->n_p[10] = t->data.ReadFloat();	 //target y
	t->n_p[11] = t->data.ReadFloat();	 //target z

	t->n_p[0] -= SERVER_VISIBLE_SPAWN_RANGE;
	t->n_p[1] -= SERVER_VISIBLE_SPAWN_RANGE;


	player_list p_s;
	player_list p_d;
	player_list p_m;

	if (w_server.area.sections[p->section_i].inflates_bounds(t->n_p) ||
		w_server.area.sections[p->section_i].cells[w_server.players[id]->cell_i].inflates_bounds(t->n_p)) {
		w_server.area.sections[p->section_i].remove_player(p);
		w_server.area.put_player(p, t->n_p);
		printf("changed cell\n");
	}

	w_server.area.collect_visible(p, t->n_p, p_s, p_d, p_m);



	t->n_s.Resize(20);
	t->n_s.WriteUInt16(0);
	t->n_s.WriteUInt16(C_W_PLAYER_MOVE_BROADCAST);
	t->n_s.WriteUInt32(id);

	uint16 p_s_next = t->n_s.NextPos();
	t->n_s.WriteInt16(p_s.count);

	uint16 p_d_next = t->n_s.NextPos();
	t->n_s.WriteInt16(p_d.count);

	uint16 p_m_next = t->n_s.NextPos();
	t->n_s.WriteInt16(p_m.count);

	t->n_s.WritePos(p_s_next);
	p_s.write_to_stream(t->n_s);

	t->n_s.WritePos(p_d_next);
	p_d.write_to_stream(t->n_s);

	t->n_s.WritePos(p_m_next);
	if (p_m.count) {

		t->n_s.WriteInt16(t->n_t[0]);  //heading
		t->n_s.WriteInt16(t->n_t[1]);  //type
		t->n_s.WriteInt16(t->n_t[2]);  //speed
		t->n_s.WriteInt16(t->n_t[3]);  //time
		//t->n_s.WriteFloat(old[0]);
		//t->n_s.WriteFloat(old[1]);
		//t->n_s.WriteFloat(old[2]);
		t->n_s.WriteFloat(a_load(w_server.players[id]->x));	  //new x
		t->n_s.WriteFloat(a_load(w_server.players[id]->y));	  //new y
		t->n_s.WriteFloat(a_load(w_server.players[id]->z));	  //new z
		t->n_s.WriteFloat(t->n_p[9]);	//target x
		t->n_s.WriteFloat(t->n_p[10]);	//target y
		t->n_s.WriteFloat(t->n_p[11]);	//target z


		p_m.write_to_stream(t->n_s);
	}

	t->n_s.WritePos(0);
	printf("%d %d %d\n", p_s.count, p_d.count, p_m.count);
	w_server_send(&t->n_s);
	return;
}

void WINAPI op_get_visible_list(worker_thread *t) {
	uint32 c_id = t->data.ReadUInt32();
	uint16 op = t->data.ReadUInt16();

	std::shared_ptr<player> p = w_server.players[c_id];
	if (p) {

		uint32 aux = (t->data._size - t->data._pos);
		Stream data;
		data.Resize(14 + (aux > 0 ? aux : 0));
		data.WriteUInt16(14 + (aux > 0 ? aux : 0));
		data.WriteUInt16(C_W_GET_VISIBLE_PLAYERS);
		data.WriteUInt16(op);
		data.WriteUInt32(c_id);
		if (aux > 0) {
			data.Write(&t->data._raw[t->data._pos], aux);
		}
		p->collect(data);
		data.WritePos(0);
		w_server_send(&data);
	}
	return;
}

