set(tests
        abstract_services
        basic_services
        decay
        dependency
        error_handling
        factory
        inheritance
        integration
        keys
        move_semantics
        readme
        scoping
)

foreach (test IN LISTS tests)
    register_test("dipp_test_${test}"
            SOURCES
            "${test}/${test}.cpp"

            LINK_LIBS
            PRIVATE
            dipp
            Boost::unit_test_framework
    )
endforeach ()