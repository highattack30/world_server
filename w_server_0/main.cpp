#include <iostream>
#include <string>

#include "win32.h"
#include "config.h"
#include "utils.h"

#include "world_server.h"


bool server_init() {
	if (!config_load()) return false;

	int32 error = 0;
	if ((error = WSAStartup(MAKEWORD(2, 2), &w_server.wsa))) {
		MB_SHOW_ERROR(std::string("WSAStartup error[" + std::to_string(error) + "]").c_str());
		return false;
	}

	if (!w_server_init())
		return false;
	
	return true;
}

void server_release() {
	WSACleanup();
	w_server_close();

}

int main(int argc, char* argv[]) {
	std::string line;
	if (!server_init()) {
		goto end_proc;
	}

	while (1) {
		std::cin >> line;

		if (stringStartsWith(line, "exit")) {
			break;
		}
	}


end_proc:
	server_release();
	return 0;
}