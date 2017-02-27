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
	w_server_release();

}

int main(int argc, char* argv[]) {

	printf("ENTER SERVER_NAME:");
	std::cin >> config::server.name;
	
	printf("ENTER AREA_ID:");
	std::cin >> config::server.area_id;
	
	printf("ENTER CHANNEL_NO.:");
	std::cin >> config::server.channel_id;
	
	printf("ENTER CONTINENT_ID:");
	std::cin >> config::server.continent_id;
	

	std::string line;
	if (!server_init()) {
		goto end_proc;
	}


	while (1) {
		std::cin >> line;

		if (stringStartsWith(line, "exit")) {
			Stream data;
			data.Resize(4);
			data.WriteUInt16(4);
			data.WriteUInt16(C_W_CLOSE);
			w_server_send(&data);

			break;
		}
		else if (stringStartsWith(line, "cls")) {
			system("cls");
		}
	}

	


end_proc:
	server_release();

	system("pause");
	return 0;
}