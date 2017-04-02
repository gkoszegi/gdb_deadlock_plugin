#include <algorithm>
#include <chrono>
#include <iostream>
#include <map>
#include <mutex>
#include <random>
#include <sstream>
#include <string>
#include <thread>
#include <vector>


/// syncronized line by line write to a stream
class sync_write
{
    public:
        sync_write(std::ostream& out)
            : ostr(out)
        {}

        template <typename T>
        friend sync_write&& operator<< (sync_write&& syncw, const T& data)
        {
            syncw.buff << data;
            return std::move(syncw);
        }

        ~sync_write()
        {
            std::lock_guard<std::mutex> l(mut);
            ostr << buff.str() << std::endl;
        }

    private:
        std::ostream& ostr;
        std::ostringstream buff;
        static std::mutex mut;
};

std::mutex sync_write::mut;


class Watchdog
{
    using time_point = std::chrono::time_point<std::chrono::steady_clock>;

    public:

        class Timeout
            : public std::runtime_error
        { 
            public:
                Timeout()
                    : std::runtime_error("Watchdog timeout")
                {}
        };

        Watchdog(const time_point::duration& checkPeriod)
            : period(checkPeriod)
        {}

        ~Watchdog()
        {
            wdThread.join();
        }

        void alive(const std::thread::id& id)
        {
            std::lock_guard<std::mutex> l(mut);
            const bool wasEmpty = last_alive.empty();
            last_alive[id] = time_point::clock::now();
            if (wasEmpty)
                wdThread = std::thread([this] { run(); });
        }

        void finished(const std::thread::id& id)
        {
            std::lock_guard<std::mutex> l(mut);
            last_alive.erase(id);
        }

        bool empty() const
        {
            std::lock_guard<std::mutex> l(mut);
            return last_alive.empty();
        }

    private:
        void run()
        {
            sync_write(std::cout) << "watchdog(" << std::this_thread::get_id() << ") started";

            bool allAlive = true;

            while (allAlive && !empty())
            {
                std::this_thread::sleep_for(period);
                auto now = time_point::clock::now();
                std::lock_guard<std::mutex> l(mut);
                for (auto&& item: last_alive)
                {
                    auto lastAlive = now - item.second;
                    if (lastAlive > period)
                    {
                        sync_write(std::cout) << "watchdog(" << std::this_thread::get_id() << ") unresponsive thread: " << item.first;
                        if (lastAlive > period * 1.5)
                            allAlive = false;
                    }
                }
            }

            sync_write(std::cout) << "watchdog(" << std::this_thread::get_id() << ") finished"
                ", allAlive: " << std::boolalpha << allAlive << ", empty: " << empty();

            if (!allAlive)
                throw Timeout();
        }

        const time_point::duration period;
        mutable std::mutex mut;
        std::map<std::thread::id, time_point> last_alive;
        std::thread wdThread;
};

class DeadlockCreator
{
    public:
        DeadlockCreator(size_t mutexCount, size_t threadCount, size_t loopCount, size_t mutexSubsetCount = 0)
            : watchdog(std::chrono::milliseconds(500))
            , mutexes(mutexCount)
            , threads(threadCount)
            , loops(loopCount)
            , mutexSubset(mutexSubsetCount > 0 && mutexSubsetCount < mutexCount ? mutexSubsetCount : mutexCount)
        {
            start();
        }

        ~DeadlockCreator()
        {
            stop();
        }

    private:
        void start()
        {
            for (auto& th: threads)
                th = std::thread([this] { run(); });
        }

        void stop()
        {
            for (auto& th: threads)
            {
                auto id = th.get_id();
                th.join();
                sync_write(std::cout) << "thread(" << id << ") joined";
            }
        }

        void run()
        {
            std::vector<std::mutex*> permutation(mutexes.size());
            std::transform(mutexes.begin(), mutexes.end(), permutation.begin(), std::addressof<std::mutex>);
            std::random_device rd;
            std::mt19937 g(rd());
            std::shuffle(permutation.begin(), permutation.end(), g);
            permutation.resize(mutexSubset);
            std::uniform_int_distribution<int> dist(1, 80);

            sync_write(std::cout) << "thread(" << std::this_thread::get_id() << ") started";

            for (size_t loop = 1; loop <= loops; ++loop)
            {
                watchdog.alive(std::this_thread::get_id());

                sync_write(std::cout) << "thread(" << std::this_thread::get_id() << ") loop " << loop << " begin";

                // lock the mutexes in order of the own random permutation
                for (std::mutex* mut: permutation)
                {
                    mut->lock();
                    std::this_thread::yield();
                }

                sync_write(std::cout) << "thread(" << std::this_thread::get_id() << ") loop " << loop << " all locks acquired";
                std::this_thread::sleep_for(std::chrono::milliseconds(dist(g)));

                // unlock the mutexes in reverse order of the own random permutation
                for (auto mut = permutation.rbegin(); mut != permutation.rend(); ++mut)
                {
                    (*mut)->unlock();
                    std::this_thread::yield();
                }

                sync_write(std::cout) << "thread(" << std::this_thread::get_id() << ") loop " << loop << " end";
            }

            watchdog.finished(std::this_thread::get_id());
            sync_write(std::cout) << "thread(" << std::this_thread::get_id() << ") finished";
        }

        Watchdog watchdog;

        std::vector<std::mutex> mutexes;
        std::vector<std::thread> threads;
        const size_t loops;
        const size_t mutexSubset;
};

int main(int arc, char* argv[])
{

    DeadlockCreator dc(20 /*mutexCount*/, 10 /*threadCount*/, 10 /*loopCount*/, 5 /*mutexSubsetCount*/);

    return 0;
}
