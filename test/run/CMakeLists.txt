if(TARGET felspar-check)
    add_test_run(felspar-check felspar-io TESTS
            allocators.cpp
            async.cpp
            basics.cpp
            cancel.cpp
            deadline.cpp
            deadline.composed.cpp
            exceptions.cpp
            pipe.cpp
            run_batch.cpp
            timers.cpp
        )
endif()
if(TARGET felspar-stress)
    add_test_run(felspar-stress felspar-io-openssl TESTS
            deadline.connect.cpp
            timers.connect.cpp
            tls.tests.cpp
        )
endif()
