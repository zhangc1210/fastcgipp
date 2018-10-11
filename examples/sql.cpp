#include <fastcgi++/request.hpp>
#include <fastcgi++/sql/connection.hpp>
#include <fastcgi++/log.hpp>
#include <random>
#include <iomanip>

class Database: public Fastcgipp::Request<char>
{
private:
    static const std::array<std::string, 8> s_strings;
    static std::random_device s_device;
    static std::uniform_int_distribution<unsigned> s_dist;
    static Fastcgipp::SQL::Connection s_connection;

    std::shared_ptr<Fastcgipp::SQL::Results<>> m_insertResult;

    std::shared_ptr<Fastcgipp::SQL::Results<
        std::chrono::time_point<std::chrono::system_clock>,
        Fastcgipp::Address,
        std::string>> m_selectResults;

    unsigned m_state;

public:
    Database():
        m_state(0)
    {}

	bool response()
	{
        using Fastcgipp::Encoding;

        switch(m_state)
        {
            case 0:
            {
                m_insertResult.reset(new Fastcgipp::SQL::Results<>);
                Fastcgipp::SQL::Query query;
                query.statement =
                    "INSERT INTO fastcgipp_example (stamp, address, string) "
                    "VALUES ($1, $2, $3);";
                query.parameters = Fastcgipp::SQL::make_Parameters(
                        std::chrono::system_clock::now(),
                        environment().remoteAddress,
                        s_strings[s_dist(s_device)]);
                query.results = m_insertResult;
                query.callback = callback();

                if(!s_connection.queue(query))
                {
                    ERROR_LOG("Unable to queue up SQL insert query")
                    errorHandler();
                    return true;
                }
                ++m_state;
                return false;
            }

            case 1:
            {
                if(m_insertResult->status() != Fastcgipp::SQL::Status::commandOk)
                {
                    ERROR_LOG("SQL insert gave unexpected status '" \
                            << Fastcgipp::SQL::statusString( \
                                m_insertResult->status()) \
                            << "' with message '" \
                            <<m_insertResult->errorMessage() << '\'')
                    errorHandler();
                    return true;
                }
                if(m_insertResult->verify() != 0)
                {
                    ERROR_LOG("SQL column verification failed: " << \
                            m_insertResult->verify())
                    errorHandler();
                    return true;
                }
                if(m_insertResult->rows() != 0)
                {
                    ERROR_LOG("SQL insert returned rows when it shouldn't have")
                    errorHandler();
                    return true;
                }
                if(m_insertResult->affectedRows() != 1)
                {
                    ERROR_LOG("SQL insert should have affected 1 row but " \
                            "instead affected " \
                            << m_insertResult->affectedRows() << '.')
                    errorHandler();
                    return true;
                }

                m_selectResults.reset(new Fastcgipp::SQL::Results<
                        std::chrono::time_point<std::chrono::system_clock>,
                        Fastcgipp::Address,
                        std::string>);
                Fastcgipp::SQL::Query query;
                query.statement =
                    "SELECT stamp, address, string FROM fastcgipp_example "
                    "ORDER BY stamp DESC LIMIT 20;";
                query.results = m_selectResults;
                query.callback = callback();

                if(!s_connection.queue(query))
                {
                    ERROR_LOG("Unable to queue up SQL select query")
                    errorHandler();
                    return true;
                }
                ++m_state;
                m_insertResult.reset();
                return false;
            }

            case 2:
            {
                if(m_selectResults->status() != Fastcgipp::SQL::Status::rowsOk)
                {
                    ERROR_LOG("SQL select gave unexpected status '" \
                            << Fastcgipp::SQL::statusString( \
                                m_selectResults->status()) \
                            << "' with message '" \
                            << m_selectResults->errorMessage() << '\'')
                    errorHandler();
                    return true;
                }
                if(m_selectResults->verify() != 0)
                {
                    ERROR_LOG("SQL column verification failed: " << \
                            m_selectResults->verify())
                    errorHandler();
                    return true;
                }
                if(m_selectResults->rows() == 0)
                {
                    ERROR_LOG("SQL select didn't return rows when it should "\
                            "have")
                    errorHandler();
                    return true;
                }

                out <<
"Content-Type: text/html; charset=iso-8859-1\r\n\r\n"
"<!DOCTYPE html>\n"
"<html lang='en'>"
    "<head>"
        "<meta charset='iso-8859-1' />"
        "<title>fastcgi++: Database</title>"
    "</head>"
    "<body>"
        "<table>"
            "<thead>"
                "<tr>"
                    "<th>Timestamp</th>"
                    "<th>IP Address</th>"
                    "<th>Random String</th>"
                "</tr>"
            "</thead>";

                for(unsigned index=0; index != m_selectResults->rows(); ++index)
                {
                    const auto& row = m_selectResults->row(index);
                    const auto timestamp = std::chrono::system_clock::to_time_t(
                            std::get<0>(row));
                    out <<
            "<tr>"
                "<td>" 
                    << std::put_time(
                            std::localtime(&timestamp),
                            "%A, %B %e %Y at %H:%M:%S %Z") <<
                "</td>"
                "<td>" << std::get<1>(row) << "</td>"
                "<td>" 
                    << Encoding::HTML << std::get<2>(row) << Encoding::NONE <<
                "</td>"
            "</tr>";
                }

                out <<
        "</table>"
    "</body>"
"</html>";
                return true;
            }
        }
    }

    static void start()
    {
        s_connection.init(
                "",
                "fastcgipp_example",
                "fastcgipp_example",
                "fastcgipp_example",
                8);
        s_connection.start();
    }

    static void terminate()
    {
        s_connection.terminate();
        s_connection.join();
    }
};

const std::array<std::string, 8> Database::s_strings
{
    "Leviathan Wakes",
    "Caliban's War",
    "Abaddon's Gate",
    "Cibola Burn",
    "Nemesis Games",
    "Babylon's Ashes",
    "Persepolis Rising",
    "Tiamat's Wrath"
};
std::random_device Database::s_device;
std::uniform_int_distribution<unsigned> Database::s_dist(0,7);
Fastcgipp::SQL::Connection Database::s_connection;

#include <fastcgi++/manager.hpp>

int main()
{
    Fastcgipp::Manager<Database> manager;
    manager.setupSignals();
    manager.listen();
    Database::start();
    manager.start();
    manager.join();
    Database::terminate();

    return 0;
}
