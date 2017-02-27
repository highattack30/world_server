#include "config.h"

#include "utils.h"
#include "server_enums.h"

#include <fstream>

config::server_c config::server;

bool WINAPI config_load() {
	if (!file_exists("config.slc")) {
		MB_SHOW_WARNING_T("Could not find 'config.slc' file!", "CONFIG FILE?");
		return  false;
	}

	std::fstream file = std::fstream("config.slc");
	if (!file.is_open()) {
		MB_SHOW_ERROR_T("Could not open 'config.slc' file!", "CONFIG FILE?");
		return false;
	}

	SYSTEM_INFO s_info;
	GetSystemInfo(&s_info);
	uint32 line_c = 0;
	std::string what;
	std::string line;
	while (!file.eof()) {
		std::getline(file, line); line_c++;
		/*if (stringStartsWith(line, "server.port")) {
			sscanf_s(line.c_str(), "server.port = %d", &config::server.port);
		}
		else*/ 
		if (stringStartsWith(line, "server.active_thread_count")) {
			if (s_info.dwNumberOfProcessors == 1) {
				printf("COULD NOT SET 'active_thread_count', NO OF CORES ON THE CPU[%d]\n", s_info.dwNumberOfProcessors);
				continue;
			}

			if (EOF == sscanf(line.c_str(), "server.active_thread_count = %d", &config::server.active_thread_count)) {
				what = "server.active_thread_count = [1," + std::to_string(s_info.dwNumberOfProcessors) + "]";
				goto error_proc;
			}

			if (config::server.active_thread_count == 0) {
				config::server.active_thread_count = 1;
			}
			else if (config::server.active_thread_count > s_info.dwNumberOfProcessors) {
				printf("YOU CAN NOT SET [%d] THREADS FOR THE ACTIVE SERVER, THATS MORE THAN CORE_COUNT/2!!\n");
				config::server.active_thread_count = s_info.dwNumberOfProcessors;
				printf("ACTIVE_SERVER_THREAD_COUNT=%d\n", config::server.active_thread_count);
			}
		}
		else if (stringStartsWith(line, "server.arbiter_ip")) {
			uint32 ip[4];
			if (4 != sscanf(line.c_str(), "server.arbiter_ip = %d.%d.%d.%d", &ip[0], &ip[1], &ip[2], &ip[3])) {
				what = "server.arbiter_ip = [1-255].[1-255].[1-255].[1-255]";
				goto error_proc;
			}
			std::string f_ip = std::to_string(ip[0]) +'.'+ std::to_string(ip[1]) + '.' + std::to_string(ip[2]) + '.' + std::to_string(ip[3]);
			if (f_ip.size() >= 16) {
				what = "server.arbiter_ip = [1-255].[1-255].[1-255].[1-255]";
				goto error_proc;
			}
			strcpy_s(config::server.arbiter_ip, f_ip.c_str());
		}
		else if (stringStartsWith(line, "server.arbiter_port")) {
			if (EOF == sscanf(line.c_str(), "server.arbiter_port = %hu", &config::server.arbiter_port) || config::server.arbiter_port == 0) {
				what = "server.arbiter_port = [positive integer]";
				goto error_proc;
			}

		}
		else if (stringStartsWith(line, "server.area_file")) {
			char buffer[_MAX_DIR];
			int32 result = sscanf(line.c_str(), "server.area_file = %s", buffer);
			if(!result || result == EOF){
				what = "invalid server.area_file value MAX_DIR_LENGTH = "+std::to_string(_MAX_DIR);
				goto error_proc;
			}
			if (!file_exists(buffer)) {
				what = "area file not found!!";
				goto error_proc;
			}
			config::server.area_file = buffer;
		}
	}
	return true;

error_proc:
	std::string error = "ERROR LINE[" + std::to_string(line_c) + "][" + line + "] -->[" + what + "]";
	MB_SHOW_ERROR_T(error.c_str(), "ERROR[CONFIG.INI PARSER]");
	return false;
}
