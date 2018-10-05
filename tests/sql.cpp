#include <iostream>
#include <memory>
#include <algorithm>
#include <array>
#include <queue>
#include <condition_variable>

#include "fastcgi++/sql/connection.hpp"

const unsigned totalQueries = 30000;
const unsigned maxQueriesSize = 1000;

struct Six
{
    int32_t one;
    int32_t two;
};
typedef std::array<char, 6> Seven;

class TestQuery
{
private:
    static Fastcgipp::SQL::Connection connection;

    std::shared_ptr<Fastcgipp::SQL::Parameters<
        int32_t,
        int64_t,
        std::string,
        float,
        double,
        Six,
        Seven,
        std::wstring>> m_parameters;

    std::shared_ptr<Fastcgipp::SQL::Results<int16_t>> m_insertResult;

    std::shared_ptr<Fastcgipp::SQL::Results<
        int32_t,
        int64_t,
        std::string,
        float,
        double,
        Six,
        Seven,
        std::wstring,
        std::wstring>> m_selectResults;

    std::shared_ptr<Fastcgipp::SQL::Results<>> m_deleteResult;

    std::function<void(Fastcgipp::Message)> m_callback;

    unsigned m_state;

    static std::map<unsigned, TestQuery> queries;

    static void callback(unsigned id, Fastcgipp::Message message);
    static std::queue<unsigned> queue;
    static std::mutex mutex;
    static std::condition_variable wake;

    bool handle();

public:
    TestQuery():
        m_state(0)
    {
    }

    static void init()
    {
        connection.init(
                "",
                "fastcgipp_test",
                "fastcgipp_test",
                "fastcgipp_test",
                8);
    }

    static void handler();
};

Fastcgipp::SQL::Connection TestQuery::connection;
std::map<unsigned, TestQuery> TestQuery::queries;
std::queue<unsigned> TestQuery::queue;
std::mutex TestQuery::mutex;
std::condition_variable TestQuery::wake;

void TestQuery::callback(unsigned id, Fastcgipp::Message message)
{
    std::lock_guard<std::mutex> lock(mutex);
    queue.push(id);
    wake.notify_one();
}

void TestQuery::handler()
{
    unsigned remaining = totalQueries;
    unsigned index=0;

    while(remaining)
    {
        while(index<totalQueries && queries.size()<maxQueriesSize)
        {
            if(queries.find(index) != queries.end())
                FAIL_LOG("Fastcgipp::SQL::Connection test fail #1")
            auto& query = queries[index];
            query.m_callback = std::bind(
                    &TestQuery::callback,
                    index,
                    std::placeholders::_1);
            query.handle();
            ++index;
        }

        unsigned id;
        {
            std::unique_lock<std::mutex> lock(mutex);
            if(queue.empty())
                wake.wait(lock);
            id = queue.front();
            queue.pop();
        }

        const auto it = queries.find(id);
        if(it == queries.end())
            FAIL_LOG("Fastcgipp::SQL::Connection test fail #2")
        if(it->second.handle())
        {
            queries.erase(it);
            --remaining;
        }
    }
}

bool TestQuery::handle()
{
    switch(m_state)
    {
        case 0:
        {
            ++m_state;
            return false;
        }

        case 1:
        {
            ++m_state;
            return false;
        }

        case 2:
        {
            ++m_state;
            return false;
        }

        case 3:
        {
            return true;
        }
    }
}

int main()
{
    // Test the SQL parameters stuff
    {
        static const int16_t zero = -1413;
        static const int32_t one = 123342945;
        static const int64_t two = -123342945112312323;
        static const std::string three = "This is a test!!34234";
        static const float four = -1656e-8;
        static const double five = 2354e15;
        static const Six six{2006, 2017};
        static const Seven seven{'a', 'b', 'c', 'd', 'e', 'f'};

        static const std::wstring eight(L"インターネット");
        static const std::array<unsigned char, 21> properEight =
        {
            0xe3, 0x82, 0xa4, 0xe3, 0x83, 0xb3, 0xe3, 0x82, 0xbf, 0xe3, 0x83,
            0xbc, 0xe3, 0x83, 0x8d, 0xe3, 0x83, 0x83, 0xe3, 0x83, 0x88
        };

        auto data(Fastcgipp::SQL::make_Parameters(
                zero,
                one,
                two,
                three,
                four,
                five,
                six,
                seven,
                eight));
        std::shared_ptr<Fastcgipp::SQL::Parameters_base> base(data);
        data.reset();

        base->build();

        if(
                *(base->oids()+0) != INT2OID ||
                *(base->sizes()+0) != 2 ||
                Fastcgipp::Protocol::BigEndian<int16_t>::read(
                    *(base->raws()+0)) != zero)
            FAIL_LOG("Fastcgipp::SQL::Parameters failed on column 0")
        if(
                *(base->oids()+1) != INT4OID ||
                *(base->sizes()+1) != 4 ||
                Fastcgipp::Protocol::BigEndian<int32_t>::read(
                    *(base->raws()+1)) != one)
            FAIL_LOG("Fastcgipp::SQL::Parameters failed on column 1")
        if(
                *(base->oids()+2) != INT8OID ||
                *(base->sizes()+2) != 8 ||
                Fastcgipp::Protocol::BigEndian<int64_t>::read(
                    *(base->raws()+2)) != two)
            FAIL_LOG("Fastcgipp::SQL::Parameters failed on column 2")
        if(
                *(base->oids()+3) != TEXTOID ||
                *(base->sizes()+3) != three.size() ||
                !std::equal(
                    three.begin(),
                    three.end(),
                    *(base->raws()+3),
                    *(base->raws()+3)+*(base->sizes()+3)))
            FAIL_LOG("Fastcgipp::SQL::Parameters failed on column 3")
        if(
                *(base->oids()+4) != FLOAT4OID ||
                *(base->sizes()+4) != 4 ||
                Fastcgipp::Protocol::BigEndian<float>::read(
                    *(base->raws()+4)) != four)
            FAIL_LOG("Fastcgipp::SQL::Parameters failed on column 4")
        if(
                *(base->oids()+5) != FLOAT8OID ||
                *(base->sizes()+5) != 8 ||
                Fastcgipp::Protocol::BigEndian<double>::read(
                    *(base->raws()+5)) != five)
            FAIL_LOG("Fastcgipp::SQL::Parameters failed on column 5")
        if(
                *(base->oids()+6) != BYTEAOID ||
                *(base->sizes()+6) != sizeof(Six) ||
                reinterpret_cast<const Six*>(*(base->raws()+6))->one != six.one ||
                reinterpret_cast<const Six*>(*(base->raws()+6))->two != six.two)
            FAIL_LOG("Fastcgipp::SQL::Parameters failed on column 6")
        if(
                *(base->oids()+7) != BYTEAOID ||
                *(base->sizes()+7) != sizeof(Seven) ||
                *reinterpret_cast<const Seven*>(*(base->raws()+7)) != seven)
            FAIL_LOG("Fastcgipp::SQL::Parameters failed on column 7")
        if(
                *(base->oids()+8) != TEXTOID ||
                *(base->sizes()+8) != properEight.size() ||
                !std::equal(
                    reinterpret_cast<const char*>(properEight.begin()),
                    reinterpret_cast<const char*>(properEight.end()),
                    *(base->raws()+8),
                    *(base->raws()+8)+*(base->sizes()+8)))
            FAIL_LOG("Fastcgipp::SQL::Parameters failed on column 8")
        for(
                const int* value = base->formats();
                value != base->formats() + base->size();
                ++value)
            if(*value != 1)
                FAIL_LOG("Fastcgipp::SQL::Parameters failed formats array")
    }

    // Test the SQL Connection
    {
        TestQuery::init();
        //TestQuery::handler();
    }

    return 0;
}
