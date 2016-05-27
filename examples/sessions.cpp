#include <fastcgi++/request.hpp>

class Sessions: public Fastcgipp::Request<char>
{
	static Fastcgipp::Http::Sessions<std::string> s_sessions;
    std::shared_ptr<const std::string> m_session;
    Fastcgipp::Http::SessionId m_sid;

	bool response()
	{
        using Fastcgipp::Encoding;
		const auto command = environment().gets.find("cmd");
        const auto sessionCookie = environment().cookies.find("sid");
        if(sessionCookie != environment().cookies.cend())
        {
            m_sid = sessionCookie->second;
            m_session = s_sessions.get(m_sid);
            if(m_session)
            {
                if(command!=environment().gets.cend()
                        && command->second=="logout")
                {
                    out << "Set-Cookie: sid=deleted; path=/; expires=Thu, "
                        "01-Jan-1970 00:00:00 GMT\n";
                    s_sessions.erase(m_sid);
                    handleNoSession();
                }
                else
                {
                    out << "Set-Cookie: sid=" << Encoding::URL
                        << sessionCookie->second << Encoding::NONE
                        << "; path=/; expires=" << s_sessions.expiration() << "\n";
                    handleSession();
                }

                return true;
            }
        }

        if(command!=environment().gets.cend() && command->second=="login")
        {
            std::shared_ptr<std::string> session(new std::string);
            const auto sessionData = environment().posts.find("data");
            if(sessionData == environment().posts.cend())
                *session = "WTF we weren't given session data!!!";
            else
                *session = sessionData->second;

            m_session = session;
            m_sid = s_sessions.generate(m_session);

            out << "Set-Cookie: sid=" << Encoding::URL << m_sid
                << Encoding::NONE << "; path=/; expires=" << s_sessions.expiration()
                << "\n";
            handleSession();
        }
        else
            handleNoSession();

		return true;
	}

    void header()
    {
        out <<
"Content-Type: text/html; charset=ISO-8859-1\r\n\r\n"
"<!DOCTYPE html>\n"
"<html lang='en'>"
	"<head>"
        "<meta charset='ISO-8859-1' />"
		"<title>fastcgi++: Sessions</title>"
	"</head>"
	"<body>";
    }

    void footer()
    {
		out <<
        "<p>There are " << s_sessions.size() << " sessions open</p>"
	"</body>"
"</html>";
    }

	void handleSession()
	{
        using Fastcgipp::Encoding;
        header();
		out <<
		"<p>We are currently in a session. The session id is "
            << m_sid << " and the session data is \"" << Encoding::HTML
            << *m_session << Encoding::NONE << "\"."
		"<p>Click <a href='?cmd=logout'>here</a> to logout</p>";
        footer();
	}

	void handleNoSession()
	{
        header();
        out <<
		"<p>We are currently not in a session.</p>"
		"<form action='?cmd=login' method='post' "
                "enctype='application/x-www-form-urlencoded' "
                "accept-charset='ISO-8859-1'>"
			"<div>"
				"Text: <input type='text' name='data' value='Hola señor, usted "
                    "me almacenó en una sesión' />"
				"<input type='submit' name='submit' value='submit' />"
			"</div>"
		"</form>";
        footer();
	}
public:
    Sessions():
        Fastcgipp::Request<char>(256)
    {}
};

Fastcgipp::Http::Sessions<std::string> Sessions::s_sessions(3600);

#include <fastcgi++/manager.hpp>

int main()
{
    Fastcgipp::Manager<Sessions> manager;
    manager.setupSignals();
    //manager.listen();
    manager.listen("127.0.0.1", "23456");
    manager.start();
    manager.join();

    return 0;
}
