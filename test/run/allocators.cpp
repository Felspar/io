#include <felspar/coro/eager.hpp>
#include <felspar/io.hpp>
#include <felspar/memory/fixed-pool.pmr.hpp>
#include <felspar/test.hpp>


using namespace std::literals;


namespace {


    auto const suite = felspar::testsuite("allocators");


    struct counting_allocator : public felspar::pmr::memory_resource {
        felspar::pmr::memory_resource *backing_allocator =
                felspar::pmr::new_delete_resource();
        std::size_t allocations{}, deallocations{};

      private:
        void *do_allocate(std::size_t bytes, std::size_t alignment) override {
            ++allocations;
            return backing_allocator->allocate(bytes, alignment);
        }

        void do_deallocate(
                void *p, std::size_t bytes, std::size_t alignment) override {
            backing_allocator->deallocate(p, bytes, alignment);
            ++deallocations;
        }

        bool do_is_equal(memory_resource const &other) const noexcept override {
            return this == &other;
        }
    };


    felspar::io::warden::task<void> sleeper(felspar::io::warden &ward) {
        co_await ward.sleep(20ms);
    }


    auto const task = suite.test("task", []() {
        felspar::io::poll_warden ward;
        ward.run(
                +[](felspar::io::warden &ward)
                        -> felspar::io::warden::task<void> {
                    felspar::test::injected check;

                    counting_allocator count;
                    felspar::io::allocator merged{ward, count};

                    felspar::io::warden::eager<> coro;
                    coro.post(sleeper, std::ref(merged));
                    check(count.allocations) == 1u;
                    check(count.deallocations) == 0u;

                    co_await coro.release();
                    check(count.allocations) == 1u;
                    check(count.deallocations) == 1u;
                });
    });


}
