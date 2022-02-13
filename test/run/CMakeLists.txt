if(TARGET check)
    add_test_run(check felspar-io TESTS
            basics.cpp
            timers.cpp
        )
endif()
if(TARGET stress)
    add_test_run(stress felspar-io TESTS
            timers.connect.cpp
        )
endif()
