if(TARGET check)
    add_test_run(check felspar-io TESTS
            basics.cpp
            timers.cpp
        )
endif()
