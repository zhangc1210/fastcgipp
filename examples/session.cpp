#include <fastcgi++/request.hpp>

class Session: public Fastcgipp::Request<char>
{
	static Fastcgipp::Http::Sessions<std::string> s_sessions;

    std::pair<std::time_t, std::shared_ptr<const std::string>> m_session;

	bool response()
	{
		const auto command = environment().gets.find("cmd");
        const auto it = environment().cookies.find("sid");
        if(it != environment().cookies.cend())
        {
            const Fastcgipp::Http::SessionId sid(it->second);
            m_session = s_sessions.get(sid);
            if(m_session.second)
            {
                if(command!=environment().gets.cend()
                        && command->second=="logout")
                {
                    out << "Set-Cookie: sid=deleted; expires=Thu, "
                        "01-Jan-1970 00:00:00 GMT;\n";
                    s_sessions.erase(sid);
                    m_session.second.reset();
                    handleNoSession();
                }
                else
                {
                    out << "Set-Cookie: sid=" << Encoding::URL << it->second
                        << Encoding::NONE << "; expires="
                        << s_session.expiration() << '\n';
                    handleSession();
                }

                return true;
            }
        }

        if(command!=environment().gets.cend() && command->second=="login")
        {
            session=sessions.generate(environment().findPost("data").value);
            out << "Set-Cookie: SESSIONID=" << encoding(URL) << session->first << encoding(NONE) << "; expires=" << sessions.expiration() << '\n';
            handleSession();
        }
        else
            handleNoSession();

		out << "<p>There are " << sessions.size() << " sessions open</p>\n\
	</body>\n\
</html>";

		// Always return true if you are done. This will let apache know we are done
		// and the manager will destroy the request and free it's resources.
		// Return false if you are not finished but want to relinquish control and
		// allow other requests to operate.
		return true;
	}

	void handleSession()
	{
		using namespace Fastcgipp;
		out << "\
Content-Type: text/html; charset=ISO-8859-1\r\n\r\n\
<!DOCTYPE html PUBLIC '-//W3C//DTD XHTML 1.0 Strict//EN' 'http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd'>\n\
<html xmlns='http://www.w3.org/1999/xhtml' xml:lang='en' lang='en'>\n\
	<head>\n\
		<meta http-equiv='Content-Type' content='text/html; charset=ISO-8859-1' />\n\
		<title>fastcgi++: Session Handling example</title>\n\
	</head>\n\
	<body>\n\
		<p>We are currently in a session. The session id is " << session->first << " and the session data is \"" << encoding(HTML) << session->second << encoding(NONE) << "\". It will expire at " << sessions.getExpiry(session) << ".</p>\n\
		<p>Click <a href='?command=logout'>here</a> to logout</p>\n";
	}

	void handleNoSession()
	{
		using namespace Fastcgipp;
		out << "\
Content-Type: text/html; charset=ISO-8859-1\r\n\r\n\
<!DOCTYPE html PUBLIC '-//W3C//DTD XHTML 1.0 Strict//EN' 'http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd'>\n\
<html xmlns='http://www.w3.org/1999/xhtml' xml:lang='en' lang='en'>\n\
	<head>\n\
		<meta http-equiv='Content-Type' content='text/html; charset=ISO-8859-1' />\n\
		<title>fastcgi++: Session Handling example</title>\n\
	</head>\n\
	<body>\n\
		<p>We are currently not in a session.</p>\n\
		<form action='?command=login' method='post' enctype='application/x-www-form-urlencoded' accept-charset='ISO-8859-1'>\n\
			<div>\n\
				Text: <input type='text' name='data' value='Hola señor, usted me almacenó en una sesión' /> \n\
				<input type='submit' name='submit' value='submit' />\n\
			</div>\n\
		</form>\n";
	}
	Sessions::iterator session;
};

Session::Sessions Session::sessions(30, 30);

// The main function is easy to set up
int main()
{
	try
	{
		// First we make a Fastcgipp::Manager object, with our request handling class
		// as a template parameter.
		Fastcgipp::Manager<Session> fcgi;
		// Now just call the object handler function. It will sleep quietly when there
		// are no requests and efficiently manage them when there are many.
		fcgi.handler();
	}
	catch(std::exception& e)
	{
		error_log(e.what());
	}
}
