
#include <map>
#include <string>

class StupidsConfig
{
public:
	StupidsConfig();
	~StupidsConfig();

	void loadConfig(const char *filename);

	static StupidsConfig &defaultInstance();

	std::string operator()(const std::string &key);

private:
	void loadLine(char *buffer);

	std::map<std::string, std::string> m_config;

	static StupidsConfig *s_instance;
};

#define StupidsConf (StupidsConfig::defaultInstance())

std::string expand_path(std::string path);

