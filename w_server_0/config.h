#ifndef W_SERVER_CONFIG_H
#define W_SERVER_CONFIG_H

#define _CRT_SECURE_NO_WARNINGS

#include "typeDefs.h"

#include <string>

struct config {
	struct server_c {
		char	arbiter_ip[16];
		uint16	arbiter_port;
		uint32	use_default_port;
		uint32	use_all_adapters;
		uint32   reconnect_loop;
		uint32  thread_count;
		uint32	auto_start;
	}static server;


};

bool WINAPI config_load();
#endif
