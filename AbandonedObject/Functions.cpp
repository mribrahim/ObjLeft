#include "Functions.h"


Functions::Functions()
{
}


Functions::~Functions()
{
}



void Functions::replace(std::string& str, const std::string& from, const std::string& to) {

	size_t start_pos = str.find(from);

	while (string::npos != start_pos)
	{
		str.replace(start_pos, from.length(), to);
		start_pos = str.find(from);
	}
}

std::string Functions::current_date() {
	time_t now = time(NULL);
	struct tm tstruct;
	char buf[40];
	tstruct = *localtime(&now);
	//format: day DD-MM-YYYY
	strftime(buf, sizeof(buf), "%A %d/%m/%Y", &tstruct);
	return buf;
}
std::string Functions::current_time() {
	time_t now = time(NULL);
	struct tm tstruct;
	char buf[40];
	tstruct = *localtime(&now);
	//format: HH:mm:ss
	strftime(buf, sizeof(buf), "%X", &tstruct);

	string str(buf);
	replace(str, ":", "_");
	return str;
}