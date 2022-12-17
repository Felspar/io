if(TARGET felspar-check)
    add_test_run(felspar-check felspar-io TESTS
            allocators.cpp
            basics.cpp
            cancel.cpp
            timers.cpp
        )
endif()
if(TARGET felspar-stress)
    add_test_run(felspar-stress felspar-io TESTS
            timers.connect.cpp
        )
endif()
