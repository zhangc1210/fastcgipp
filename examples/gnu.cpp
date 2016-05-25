#include <fastcgi++/request.hpp>
#include <iomanip>

class Gnu: public Fastcgipp::Request<wchar_t>
{
    static const std::vector<std::string> locales;
    static const std::vector<std::vector<std::wstring>> catalogues;
    static const unsigned char gnuPng[];
    static const size_t gnuPngSize = 58587;
    static const std::time_t startTimestamp;
    static const std::tm startTime;

    inline void image()
    {
		if(startTimestamp <= environment().ifModifiedSince)
			out << "Status: 304 Not Modified\r\n\r\n";
        else
        {
            out << "Last-Modified: "
                << std::put_time(&startTime, L"%a, %d %b %Y %H:%M:%S GMT\n");
            out << L"Content-Length: " << gnuPngSize << '\n';
            out << L"Content-Type: image/png\r\n\r\n";
            dump(gnuPng, gnuPngSize);
        }
    }

    inline void html()
    {
        const unsigned locale = pickLocale(locales);
        const std::string language = locales[locale].substr(0,2);
        const std::vector<std::wstring>& catalogue(catalogues[locale]);

        if(
                locale==environment().etag
                && startTimestamp<=environment().ifModifiedSince)
			out << "Status: 304 Not Modified\r\n\r\n";
        else
        {
            out << "Last-Modified: "
                << std::put_time(&startTime, L"%a, %d %b %Y %H:%M:%S GMT\n");
            out << L"Etag: " << locale << '\n';
            out << L"Content-Type: text/html; charset=utf-8\n";
            out << L"Content-Language: " << language.c_str() << L"\r\n\r\n";

            setLocale(locales[locale]);

            out <<
L"<!DOCTYPE html>\n"
L"<html lang='" << language.c_str() << L"'>"
    L"<head>"
        L"<meta charset='utf-8' />"
        L"<title>fastcgi++: " << catalogue[0] << L"</title>"
    L"</head>"
    L"<body>"
        L"<h1>" << catalogue[1] << L"</h1>"
        L"<figure>"
            L"<img src='" << environment().scriptName << L"/gnu.png' alt='"
                << catalogue[2] << L"'>"
            L"<figcaption>" << catalogue[3] << gnuPngSize
                << catalogue[4] << std::put_time(&startTime, L"%c")
                << L". </figcaption>"
        L"</figure>"
    L"</body>"
L"</html>";
        }
    }

	bool response()
	{
        if(
                environment().pathInfo.size() == 1
                && environment().pathInfo[0] == L"gnu.png")
            image();
        else
            html();

		return true;
	}
};

const std::vector<std::string> Gnu::locales
{
    "en_CA",
    "en_US",
    "fr_CA",
    "fr_FR",
    "zh_CN",
    "de_DE"
};

const std::vector<std::vector<std::wstring>> Gnu::catalogues
{
    {
        L"Showing the colourless GNU",
        L"This is a header",
        L"The GNU Logo",
        L"Figure 1: This GNU logo is ",
        L" bytes. It was last modified "
    },
    {
        L"Showing the colorless GNU",
        L"This is a header",
        L"The GNU Logo",
        L"Figure 1: This GNU logo is ",
        L" bytes. It was last modified "
    },
    {
        L"Montrant le GNU incolore",
        L"Ceci est un en-tête",
        L"Le logo GNU",
        L"Figure 1: Ce logo GNU est de ",
        L" octets. Il a été modifié "
    },
    {
        L"Montrant le GNU incolore",
        L"Ceci est un en-tête",
        L"Le logo GNU",
        L"Figure 1: Ce logo GNU est de ",
        L" octets. Il a été modifié "
    },
    {
        L"顯示無色GNU",
        L"這是一個標頭",
        L"GNU的標誌",
        L"圖1 ：這GNU標誌是",
        L"字節。最後一次修改"
    },
    {
        L"Angezeigt wird die farblose GNU",
        L"Dies ist ein Kopf",
        L"Das GNU- Logo",
        L"Abbildung 1: Das GNU -Logo ist ",
        L" Bytes. Es wurde zuletzt geändert "
    }
};

const unsigned char Gnu::gnuPng[] =
#include "gnu.png.hpp"

const std::time_t Gnu::startTimestamp = std::time(nullptr);
const std::tm Gnu::startTime = *std::gmtime(&startTimestamp);

#include <fastcgi++/manager.hpp>

int main()
{
    Fastcgipp::Manager<Gnu> manager;
    manager.setupSignals();
    manager.listen();
    manager.start();
    manager.join();

    return 0;
}
