#include <iostream>
#include <stdio.h>
#include <time.h> 

using namespace std;

#pragma once
class Functions
{
public:
	Functions();
	~Functions();

	void replace(std::string& str, const std::string& from, const std::string& to);
	std::string current_date();
	std::string current_time();
	bool GenerateDirectory(std::string path);


};

