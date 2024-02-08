#pragma once

#include <vector>
#include <string>
#include <fstream>

class FileManager
{
public:
	static std::vector<char> ReadRawBytes(const std::string& filename);
};

