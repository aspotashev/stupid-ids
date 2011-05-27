
#include <exception>

//-------- Working with stupids-server.rb over TCP/IP --------

class TpHashNotFoundException : public std::exception
{
	virtual const char *what() const throw();
};

std::vector<int> get_min_ids_by_tp_hash(const char *tp_hash);

