
#include <map>
#include <string>

// TBD: use Boost or another 3rd-party lib?
//     http://stackoverflow.com/questions/6175502/how-to-parse-ini-file-with-boost
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

