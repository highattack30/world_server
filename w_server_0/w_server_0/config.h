#ifndef W_SERVER_CONFIG_H
#define W_SERVER_CONFIG_H

#define _CRT_SECURE_NO_WARNINGS

#include "typeDefs.h"

#include <string>

struct config {
	struct server_c {
		char	arbiter_ip[16];
		uint16	arbiter_port;
		uint32  active_thread_count;
		uint16	port;
		uint32	area_id;
		uint32	continent_id;
		uint32	channel_id;
		std::string name;
		std::string area_file;
	}static server;


};

bool WINAPI config_load();
#endif
