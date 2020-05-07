#include "fastcgi++/log.hpp"
#include "fastcgi++/curler.hpp"

#include "fastcgi++/config.hpp"
#if defined FASTCGIPP_UNIX || defined FASTCGIPP_LINUX
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>
#include <sstream>
#include <sys/stat.h>
#include <fcntl.h>

#include <cstring>
#include <sstream>
#include <iomanip>

unsigned int openfds()
{
    std::ostringstream ss;
    ss << "/proc/" << getpid() << "/fd";

    DIR* const directory = opendir(ss.str().c_str());
    dirent* file;
    unsigned int count = 0;
    char path[1024];

    ss << '/';
    while((file = readdir(directory)) != nullptr) {
        ++count;
        auto c = readlink((ss.str() + file->d_name).c_str(), path, sizeof(path) - 1);
        if(c > 0) {
            path[c] = 0;
            INFO_LOG("fd " << file->d_name << " -> " << path);
        } else {
            INFO_LOG("fd " << file->d_name)
        }
    }

    closedir(directory);
    return count;
}
#elif
unsigned int openfds()
{
    return 0;
}
#endif

#include <condition_variable>
#include <mutex>

std::condition_variable wake;
std::mutex mutex;
int remaining=16;
int active=0;
const int maxActive=8;

template<class charT>
void callback(
        Fastcgipp::Curl<charT> curl,
        const std::vector<std::basic_string<charT>> proper,
        Fastcgipp::Message message)
{
    if(curl.responseCode() != 200)
        FAIL_LOG("Got response code = " << curl.responseCode() << " instead of 200");
    const std::basic_string<charT> data(curl.data(), curl.data()+curl.dataSize());

    for(const auto& prop: proper)
        if(data.find(prop) == std::basic_string<charT>::npos)
            FAIL_LOG("Got bad response: '" << prop.c_str() << "' not in '" << data.c_str() << "'");

    std::lock_guard<std::mutex> lock(mutex);
    --remaining;
    --active;
    wake.notify_all();
}

int main()
{
    std::setlocale(LC_ALL, "en_US.utf8");
    const auto initialFds = openfds();

    {
        Fastcgipp::Curler curler(maxActive);
        curler.start();

        // Test wide character POST
        int id=0;
        remaining=16;
        active=0;
        while(remaining > 0)
        {
            if(remaining-active > 0)
            {
                std::wstringstream ss;
                std::vector<std::wstring> proper;

                ss << "\"args\":{}";
                proper.push_back(ss.str());
                ss.clear();
                ss.str(std::wstring());

                ss << "\"data\":\"This is a test" << L" 黑暗森林 " << std::setfill(wchar_t('0')) << std::setw(5) << id << L" Привет" << "\"";
                proper.push_back(ss.str());
                ss.clear();
                ss.str(std::wstring());

                ss << "\"content-length\":\"46\"";
                proper.push_back(ss.str());
                ss.clear();
                ss.str(std::wstring());

                Fastcgipp::Curl<wchar_t> curl;
                curl << "This is a test" << L" 黑暗森林 " << std::setfill(wchar_t('0')) << std::setw(5) << id++ << L" Привет";
                curl.setCallback(std::bind(callback<wchar_t>, curl, proper, std::placeholders::_1));
                curl.setUrl("https://postman-echo.com/post");
                curl.addHeader("Content-Type: text/plain; charset=utf-8");
                curl.addHeader("Accept-Charset: utf-8");
                curler.queue(curl);
            }

            std::unique_lock<std::mutex> lock(mutex);
            if(++active == maxActive)
                wake.wait(lock);
        }

        // Test narrow character POST
        id=0;
        remaining=16;
        active=0;
        while(remaining > 0)
        {
            if(remaining-active > 0)
            {
                std::stringstream ss;
                std::vector<std::string> proper;

                ss << "\"args\":{}";
                proper.push_back(ss.str());
                ss.clear();
                ss.str(std::string());

                ss << "\"data\":\"This is a test " << std::setfill('0') << std::setw(5) << id << "\"";
                proper.push_back(ss.str());
                ss.clear();
                ss.str(std::string());

                ss << "\"content-length\":\"20\"";
                proper.push_back(ss.str());
                ss.clear();
                ss.str(std::string());

                Fastcgipp::Curl<char> curl;
                curl << "This is a test " << std::setfill('0') << std::setw(5) << id++;
                curl.setCallback(std::bind(callback<char>, curl, proper, std::placeholders::_1));
                curl.setUrl("https://postman-echo.com/post");
                curl.addHeader("Content-Type: text/plain; charset=utf-8");
                curl.addHeader("Accept-Charset: utf-8");
                curler.queue(curl);
            }

            std::unique_lock<std::mutex> lock(mutex);
            if(++active == maxActive)
                wake.wait(lock);
        }

        // Test wide character GET
        id=0;
        remaining=16;
        active=0;
        while(remaining > 0)
        {
            if(remaining-active > 0)
            {
                std::wstringstream ss;
                std::vector<std::wstring> proper;

                ss << "\"args\":{\"foo1\":\"bar1\",\"foo2\":\"bar2\",\"id\":\"" << id << "\"}";
                proper.push_back(ss.str());
                ss.clear();
                ss.str(std::wstring());

                Fastcgipp::Curl<wchar_t> curl;
                curl.setCallback(std::bind(callback<wchar_t>, curl, proper, std::placeholders::_1));
                curl.setUrl("https://postman-echo.com/get?foo1=bar1&foo2=bar2&id="+std::to_string(id++));
                curl.addHeader("Content-Type: text/plain; charset=utf-8");
                curl.addHeader("Accept-Charset: utf-8");
                curler.queue(curl);
            }

            std::unique_lock<std::mutex> lock(mutex);
            if(++active == maxActive)
                wake.wait(lock);
        }

        // Test narrow character GET
        id=0;
        remaining=16;
        active=0;
        while(remaining > 0)
        {
            if(remaining-active > 0)
            {
                std::stringstream ss;
                std::vector<std::string> proper;

                ss << "\"args\":{\"foo1\":\"bar1\",\"foo2\":\"bar2\",\"id\":\"" << id << "\"}";
                proper.push_back(ss.str());
                ss.clear();
                ss.str(std::string());

                Fastcgipp::Curl<char> curl;
                curl.setCallback(std::bind(callback<char>, curl, proper, std::placeholders::_1));
                curl.setUrl("https://postman-echo.com/get?foo1=bar1&foo2=bar2&id="+std::to_string(id++));
                curl.addHeader("Content-Type: text/plain; charset=utf-8");
                curl.addHeader("Accept-Charset: utf-8");
                curler.queue(curl);
            }

            std::unique_lock<std::mutex> lock(mutex);
            if(++active == maxActive)
                wake.wait(lock);
        }

        curler.terminate();
        curler.join();
    }

    if(openfds() != initialFds)
        FAIL_LOG("There are leftover file descriptors after they should all "\
                "have been closed");
    return 0;
}
