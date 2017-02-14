#ifndef UTILS_H
#define UTILS_H

#include "typeDefs.h"
#include "win32.h"


#define MB_SHOW_ERROR(a) MessageBox(GetConsoleWindow() , a ,"Error", MB_OK | MB_ICONERROR)
#define MB_SHOW_WARNING(a) MessageBox(GetConsoleWindow() , a ,"Error", MB_OK | MB_ICONASTERISK)

#define MB_SHOW_ERROR_T(a,b) MessageBox(GetConsoleWindow() , a ,b, MB_OK | MB_ICONERROR)
#define MB_SHOW_WARNING_T(a,b) MessageBox(GetConsoleWindow() , a ,b, MB_OK | MB_ICONASTERISK)

bool WINAPI file_exists(const char*);

bool stringStartsWith(std::string str, std::string with, bool ignoreWhiteSpaces = true);
std::string string_split_get_right(std::string, char);

#endif
