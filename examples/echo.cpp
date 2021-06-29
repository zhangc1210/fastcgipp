
#include <fastcgi++/request.hpp>
#include <iomanip>

class Echo: public Fastcgipp::Request<char>
{
    //! [Request definition]
    //! [Max POST]
public:
    Echo():
        Fastcgipp::Request<char>(5*1024)
    {}

private:
    bool response()
    {
		std::string str = "Status:200 \r\n\r\nit's work";
		dump(str.c_str(),str.length());
		return true;
    }
};

#include <fastcgi++/manager.hpp>
#include <fastcgi++/log.hpp>
int main()
{
	if (!Fastcgipp::Socket::Startup())
	{
		return -1;
	}
	::Fastcgipp::Logging::suppress = true;
    Fastcgipp::Manager<Echo> manager;
    manager.setupSignals();
    manager.listen(0,9000);
    manager.start();
    manager.join();
	Fastcgipp::Socket::Cleanup();//释放资源的操作
    return 0;
}
//! [Finish]
