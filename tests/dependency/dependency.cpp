#define BOOST_TEST_MODULE Test_Dependency

#include <boost/test/included/unit_test.hpp>
#include <dipp/dipp.hpp>
#include <mutex>

BOOST_AUTO_TEST_SUITE(Test_Dependency)

//

struct A
{
    bool* a_destroyed = nullptr;

    A(bool* a_destroyed) : a_destroyed(a_destroyed)
    {
        *a_destroyed = false;
    }

    A(const A&)            = delete;
    A& operator=(const A&) = delete;

    A(A&& o) : a_destroyed(o.a_destroyed)
    {
        o.a_destroyed = nullptr;
    }

    A& operator=(A&&) = delete;

    ~A()
    {
        if (a_destroyed)
        {
            *a_destroyed = true;
        }
    }
};

struct B
{
    bool* b_destroyed = nullptr;
    A&    a;

    B(A& a, bool* b_destroyed) : a(a), b_destroyed(b_destroyed)
    {
        *b_destroyed = false;
    }

    B(const B&)            = delete;
    B& operator=(const B&) = delete;

    B(B&& o) : a(o.a), b_destroyed(o.b_destroyed)
    {
        o.b_destroyed = nullptr;
    }

    B& operator=(B&&) = delete;

    ~B()
    {
        // assert that A is still alive
        BOOST_CHECK(!a.a_destroyed || !*a.a_destroyed);

        if (b_destroyed)
        {
            *b_destroyed = true;
        }
    }
};

struct C
{
    bool* c_destroyed = nullptr;
    A&    a;
    B&    b;

    C(A& a, B& b, bool* c_destroyed) : a(a), b(b), c_destroyed(c_destroyed)
    {
        *c_destroyed = false;
    }

    C(const C&)            = delete;
    C& operator=(const C&) = delete;

    C(C&& o) : a(o.a), b(o.b), c_destroyed(o.c_destroyed)
    {
        o.c_destroyed = nullptr;
    }

    C& operator=(C&&) = delete;

    ~C()
    {
        // assert that A and B are still alive
        BOOST_CHECK(!a.a_destroyed || !*a.a_destroyed);
        BOOST_CHECK(!b.b_destroyed || !*b.b_destroyed);

        if (c_destroyed)
        {
            *c_destroyed = true;
        }
    }
};

using ASingleton = dipp::injected<A, dipp::service_lifetime::singleton>;
using BSingleton = dipp::injected<B, dipp::service_lifetime::singleton, dipp::dependency<ASingleton>>;
using CSingleton = dipp::injected<C, dipp::service_lifetime::singleton, dipp::dependency<ASingleton, BSingleton>>;

//

BOOST_AUTO_TEST_CASE(Test_Lifetime_Dependency)
{
    bool a_destroyed = true;
    bool b_destroyed = true;
    bool c_destroyed = true;

    {
        dipp::default_service_collection collection;

        collection.add(ASingleton::descriptor_type([&a_destroyed](auto&) { return dipp::make_any<A>(&a_destroyed); }));

        collection.add(
            BSingleton::descriptor_type([&a_destroyed, &b_destroyed](auto& scope)
                                        { return dipp::make_any<B>(scope.get<ASingleton>(), &b_destroyed); }));

        collection.add(CSingleton::descriptor_type(
            [&a_destroyed, &b_destroyed, &c_destroyed](auto& scope)
            { return dipp::make_any<C>(scope.get<ASingleton>(), scope.get<BSingleton>(), &c_destroyed); }));

        dipp::default_service_provider services(std::move(collection));

        auto a = services.get<ASingleton>();
        auto b = services.get<BSingleton>();
        auto c = services.get<CSingleton>();

        (void)a;
        (void)b;
        (void)c;

        BOOST_CHECK(!a_destroyed);
        BOOST_CHECK(!b_destroyed);
        BOOST_CHECK(!c_destroyed);
    }

    BOOST_CHECK(a_destroyed);
    BOOST_CHECK(b_destroyed);
    BOOST_CHECK(c_destroyed);
}

BOOST_AUTO_TEST_SUITE_END()
