if(TARGET felspar-check)
    add_test_run(felspar-check felspar-io TESTS
            allocators.cpp
            basics.cpp
            cancel.cpp
            pipe.cpp
            run_batch.cpp
            timers.cpp
        )
endif()
if(TARGET felspar-stress)
    add_test_run(felspar-stress felspar-io-openssl TESTS
            timers.connect.cpp
            tls.tests.cpp
        )
endif()
