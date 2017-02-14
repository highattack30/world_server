#include "config.h"

#include "utils.h"
#include "server_enums.h"

#include <fstream>

config::server_c config::server;

bool WINAPI config_load() {
	if (!file_exists("config.slc")) {
		MB_SHOW_WARNING_T("Could not find 'config.slc' file!","CONFIG FILE?");
		return  false;
	}

	std::fstream file = std::fstream("config.slc");
	if (!file.is_open()) {
		MB_SHOW_ERROR_T("Could not open 'config.slc' file!", "CONFIG FILE?");
		return false;
	}

	uint32 line_c = 0;
	std::string what;
	std::string line;
	while (!file.eof()) {
		std::getline(file, line); line_c++;
		if (stringStartsWith(line, "server.use_all_adapters")) {
			sscanf_s(line.c_str(), "server.use_all_adapters = %d", &config::server.use_all_adapters);
			if (config::server.use_all_adapters != 0 && config::server.use_all_adapters != 1) {
				what = "server.use_local_ip = [0/1]";
				goto error_proc;
			}
		}
		else if (stringStartsWith(line, "server.use_default_port")) {
			sscanf_s(line.c_str(), "server.use_default_port = %d", &config::server.use_default_port);
			if (config::server.use_default_port != 0 && config::server.use_default_port != 1) {
				what = "server.use_default_port = [0/1]";
				goto error_proc;
			}
		}
		else if (stringStartsWith(line, "server.arbiter_ip")) {
			if (EOF == sscanf(line.c_str(), "server.arbiter_ip = %s", config::server.arbiter_ip)) {
				what = "server.arbiter_ip = [1-255].[1-255].[1-255].[1-255]";
				goto error_proc;
			}

		}
		else if (stringStartsWith(line, "server.arbiter_port")) {
			if (EOF == sscanf(line.c_str(), "server.arbiter_port = %hu", &config::server.arbiter_port) || config::server.arbiter_port == 0) {
				what = "server.arbiter_port = [positive integer]";
				goto error_proc;
			}

		}
		else if (stringStartsWith(line, "server.reconnect_loop")) {
			sscanf_s(line.c_str(), "server.reconnect_loop = %d", &config::server.reconnect_loop);
			if (config::server.reconnect_loop != 0 && config::server.reconnect_loop != 1) {
				what = "server.reconnect_loop = [0/1]";
				goto error_proc;
			}
		}
		else if (stringStartsWith(line, "server.thread_count")) {
			if (EOF == sscanf(line.c_str(), "server.thread_count = %d", &config::server.thread_count) || config::server.thread_count == 0 || config::server.thread_count > SERVER_MAX_THREAD_COUNT) {
				what = "server.thread_count = [1," + std::to_string(SERVER_MAX_THREAD_COUNT) + "]";
				goto error_proc;
			}

		}
		else if (stringStartsWith(line, "server.server.auto_start")) {
			sscanf_s(line.c_str(), "server.server.auto_start = %d", &config::server.auto_start);
			if (config::server.auto_start != 0 && config::server.auto_start != 1) {
				what = "server.server.auto_start = [0/1]";
				goto error_proc;
			}
		}
		
	}
	return true;


error_proc:
	std::string error = "ERROR LINE[" + std::to_string(line_c) + "][" + line + "] -->[" + what + "]";
	MB_SHOW_ERROR_T(error.c_str() , "ERROR[CONFIG.INI PARSER]");
	return false;
}
