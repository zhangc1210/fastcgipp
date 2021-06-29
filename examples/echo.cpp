
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
		using Fastcgipp::Encoding;
		//! [Encoding using]

		//! [Header]
		out << "Content-Type: text/html; charset=utf-8\r\n\r\n";
		//! [Header]

		//! [HTML]
		out <<
			"<!DOCTYPE html>\n"
			"<html>"
			"<head>"
			"<meta charset='utf-8' />"
			"<title>fastcgi++: Echo</title>"
			"</head>"
			"<body>"
			"<h1>Echo</h1>";
		//! [HTML]

		//! [Environment]
		out <<
			"<h2>Environment Parameters</h2>"
			"<p>"
			"<b>FastCGI Version:</b> "
			<< Fastcgipp::Protocol::version << "<br />"
			"<b>fastcgi++ Version:</b> " << Fastcgipp::version << "<br />"
			"<b>Hostname:</b> " << Encoding::HTML << environment().host
			<< Encoding::NONE << "<br />"
			"<b>Origin Server:</b> " << Encoding::HTML << environment().origin
			<< Encoding::NONE << "<br />"
			"<b>User Agent:</b> " << Encoding::HTML << environment().userAgent
			<< Encoding::NONE << "<br />"
			"<b>Accepted Content Types:</b> " << Encoding::HTML
			<< environment().acceptContentTypes << Encoding::NONE
			<< "<br />"
			"<b>Accepted Languages:</b> " << Encoding::HTML;
		if (!environment().acceptLanguages.empty())
		{
			auto language = environment().acceptLanguages.cbegin();
			while (true)
			{
				out << language->c_str();
				++language;
				if (language == environment().acceptLanguages.cend())
					break;
				out << ',';
			}
		}
		out << Encoding::NONE << "<br />"
			"<b>Accepted Characters Sets:</b> " << Encoding::HTML
			<< environment().acceptCharsets << Encoding::NONE << "<br />"
			"<b>Referer:</b> " << Encoding::HTML << environment().referer
			<< Encoding::NONE << "<br />"
			"<b>Content Type:</b> " << Encoding::HTML
			<< environment().contentType << Encoding::NONE << "<br />"
			"<b>Root:</b> " << Encoding::HTML << environment().root
			<< Encoding::NONE << "<br />"
			"<b>Script Name:</b> " << Encoding::HTML
			<< environment().scriptName << Encoding::NONE << "<br />"
			"<b>Request URI:</b> " << Encoding::HTML
			<< environment().requestUri << Encoding::NONE << "<br />"
			"<b>Request Method:</b> " << environment().requestMethod
			<< "<br />"
			"<b>Content Length:</b> " << environment().contentLength
			<< " bytes<br />"
			"<b>Keep Alive Time:</b> " << environment().keepAlive
			<< " seconds<br />"
			"<b>Server Address:</b> " << environment().serverAddress
			<< "<br />"
			"<b>Server Port:</b> " << environment().serverPort << "<br />"
			"<b>Client Address:</b> " << environment().remoteAddress << "<br />"
			"<b>Client Port:</b> " << environment().remotePort << "<br />"
			"<b>Etag:</b> " << environment().etag << "<br />"
			"<b>If Modified Since:</b> " << Encoding::HTML
			<< std::put_time(std::gmtime(&environment().ifModifiedSince),
				"%a, %d %b %Y %H:%M:%S") << Encoding::NONE <<
			"</p>";
		//! [Environment]

		//! [Path Info]
		out <<
			"<h2>Path Info</h2>";
		//! [Other Environment]
		out <<
			"<h2>Other Environment Parameters</h2>";
		if (!environment().others.empty())
			for (const auto& other : environment().others)
				out << "<b>" << Encoding::HTML << other.first << Encoding::NONE
				<< ":</b> " << Encoding::HTML << other.second
				<< Encoding::NONE << "<br />";
		else
			out <<
			"<p>No Other Environment Parameters</p>";
		//! [Other Environment]

		//! [GET Data]
		out <<
			"<h2>GET Data</h2>";
		if (!environment().gets.empty())
			for (const auto& get : environment().gets)
				out << "<b>" << Encoding::HTML << get.first << Encoding::NONE
				<< ":</b> " << Encoding::HTML << get.second
				<< Encoding::NONE << "<br />";
		else
			out <<
			"<p>No GET data</p>";
		//! [GET Data]

		//! [POST Data]
		out <<
			"<h2>POST Data</h2>";
		if (!environment().posts.empty())
			for (const auto& post : environment().posts)
				out << "<b>" << Encoding::HTML << post.first << Encoding::NONE
				<< ":</b> " << Encoding::HTML << post.second
				<< Encoding::NONE << "<br />";
		else
			out <<
			"<p>No POST data</p>";
		//! [POST Data]

		//! [Cookies]
		out <<
			"<h2>Cookies</h2>";
		if (!environment().cookies.empty())
			for (const auto& cookie : environment().cookies)
				out << "<b>" << Encoding::HTML << cookie.first
				<< Encoding::NONE << ":</b> " << Encoding::HTML
				<< cookie.second << Encoding::NONE << "<br />";
		else
			out <<
			"<p>No Cookies</p>";
		//! [Cookies]

		//! [Files]
		out <<
			"<h2>Files</h2>";
		if (!environment().files.empty())
		{
			for (const auto& file : environment().files)
			{
				out <<
					"<h3>" << Encoding::HTML << file.first << Encoding::NONE << "</h3>"
					"<p>"
					"<b>Filename:</b> " << Encoding::HTML << file.second.filename
					<< Encoding::NONE << "<br />"
					"<b>Content Type:</b> " << Encoding::HTML
					<< file.second.contentType << Encoding::NONE << "<br />"
					"<b>Size:</b> " << file.second.size << "<br />"
					"<b>Data:</b>"
					"</p>"
					"<pre>";
				//! [Files]
				//! [Dump]
				dump(file.second.data.get(), file.second.size);
				out <<
					"</pre>";
			}
		}
		else
			out <<
			"<p>No files</p>";
		//! [Dump]

		//! [Finish]
		out <<
			"</body>"
			"</html>";
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
	if (!manager.listen(0, 9000))
	{
		return -1;
	}
	manager.start();
	manager.join();
	Fastcgipp::Socket::Cleanup();//释放资源的操作
    return 0;
}
//! [Finish]
