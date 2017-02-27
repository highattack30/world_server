#include "utils.h"

#include <sstream>

bool WINAPI file_exists(const char *szPath){
	DWORD dwAttrib = GetFileAttributes(szPath);

	return (dwAttrib != INVALID_FILE_ATTRIBUTES &&
		!(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

bool stringStartsWith(std::string a, std::string with, bool ignoreWhiteSpaces)
{
	if (a.size() < with.size())
		return false;
	if (with.size() == 0)
		return true;

	std::string result;
	if (ignoreWhiteSpaces)
	{
		for (size_t i = 0; i < a.size(); i++)
		{
			if (a[i] != ' ')
				result += a[i];
		}
	}
	else result = a;

	for (size_t i = 0; i < with.size(); i++)
	{
		if (result[i] != with[i])
		{
			return false;
		}
	}

	return true;
}

std::string string_split_get_right(std::string line, char c)
{
	std::string out;
	bool other_side = false;
	for (size_t i = 0; i < line.size(); i++)
	{
		if (other_side) out += line[i];

		if (line[i] == c) other_side = true;
	}

	return std::move(out);
}

const char * WINAPI get_first_machine_ip(){
	WORD wVersionRequested;
	WSADATA wsaData;
	char name[255];
	PHOSTENT hostinfo;
	wVersionRequested = MAKEWORD(1, 1);
	char *ip;

	if (WSAStartup(wVersionRequested, &wsaData) == 0)
		if (gethostname(name, sizeof(name)) == 0)
		{
			if ((hostinfo = gethostbyname(name)) != NULL)
			{
				int nCount = 0;
				return  inet_ntoa(*(
					struct in_addr *)hostinfo->h_addr_list[nCount]);
			}
		}
	return nullptr;
}
