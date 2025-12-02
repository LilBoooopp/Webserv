#include "Config.hpp"

void Config::setError(size_t line, const std::string &msg)
{
	if (!_isError)
	{
		_isError = true;
		_ErrorLine = line;
		_ErrorMsg = msg;
	}
}
