#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <chrono>
#include <cstdlib>

// ─────────────────────────────────────────────
//  Shared state
// ─────────────────────────────────────────────
struct SharedData {
    std::vector<int>      array;            // the shared array
    int                   arraySize = 0;
    int                   numMarkers = 0;

    // Synchronisation
    std::mutex            mtx;

    // main → markers: "start working"
    bool                  startSignal = false;
    std::condition_variable cvStart;

    // markers → main: "I'm blocked"
    std::atomic<int>      blockedCount{0};   // how many markers have signalled blocked
    std::condition_variable cvAllBlocked;    // main waits on this

    // main → markers: "continue" or "terminate"
    // Each marker waits on its own condition
    std::vector<bool>             continueSignal; // true = continue, checked after wakeup
    std::vector<bool>             terminateSignal;
    std::vector<std::condition_variable> cvMarker; // one per marker

    // marker finished
    std::atomic<int>      finishedCount{0};
};

// ─────────────────────────────────────────────
//  Marker thread function
// ─────────────────────────────────────────────
void markerThread(SharedData& sd, int id /* 1-based */) {
    // 1. Wait for start signal
    {
        std::unique_lock<std::mutex> lk(sd.mtx);
        sd.cvStart.wait(lk, [&]{ return sd.startSignal; });
    }

    // 2. Seed RNG with own id
    srand(static_cast<unsigned>(id));

    int idx0 = id - 1; // 0-based index for cvMarker / signals

    while (true) {
        // 3. Try to mark elements until blocked
        bool blocked = false;
        int  markedThisRound = 0;

        while (!blocked) {
            int r = rand() % sd.arraySize;

            if (sd.array[r] == 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
                sd.array[r] = id;
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
                ++markedThisRound;
            } else {
                // Count how many elements this marker has marked in total
                int totalMarked = 0;
                for (int v : sd.array)
                    if (v == id) ++totalMarked;

                std::cout << "[Marker " << id << "] blocked: marked=" << totalMarked
                          << ", blocked at index=" << r << "\n";

                blocked = true;

                // 3.4.2 Signal main that we're blocked
                {
                    std::unique_lock<std::mutex> lk(sd.mtx);
                    sd.blockedCount.fetch_add(1);
                    sd.cvAllBlocked.notify_one();
                }

                // 3.4.3 Wait for main's response
                {
                    std::unique_lock<std::mutex> lk(sd.mtx);
                    sd.cvMarker[idx0].wait(lk, [&]{
                        return sd.continueSignal[idx0] || sd.terminateSignal[idx0];
                    });
                }
            }
        }

        // 4. Terminate?
        if (sd.terminateSignal[idx0]) {
            // 4.1 Zero out all elements this marker marked
            for (int& v : sd.array)
                if (v == id) v = 0;
            sd.finishedCount.fetch_add(1);
            return;
        }

        // 5. Continue
        {
            std::unique_lock<std::mutex> lk(sd.mtx);
            sd.continueSignal[idx0] = false;
        }
        // loop back to step 3
    }
}

// ─────────────────────────────────────────────
//  Helpers
// ─────────────────────────────────────────────
static void printArray(const SharedData& sd) {
    std::cout << "Array: [";
    for (int i = 0; i < sd.arraySize; ++i)
        std::cout << sd.array[i] << (i + 1 < sd.arraySize ? ", " : "");
    std::cout << "]\n";
}

// ─────────────────────────────────────────────
//  Main thread
// ─────────────────────────────────────────────
int main() {
    SharedData sd;

    // 1. Read array size
    std::cout << "Enter array size: ";
    std::cin >> sd.arraySize;

    // 2. Init array with zeros
    sd.array.assign(sd.arraySize, 0);

    // 3. Read number of marker threads
    std::cout << "Enter number of marker threads: ";
    std::cin >> sd.numMarkers;

    // Resize per-marker vectors
    sd.continueSignal.assign(sd.numMarkers, false);
    sd.terminateSignal.assign(sd.numMarkers, false);
    // condition_variable is not copyable, so construct in-place
    sd.cvMarker = std::vector<std::condition_variable>(sd.numMarkers);

    // 4. Launch marker threads
    std::vector<std::thread> markers;
    markers.reserve(sd.numMarkers);
    for (int i = 1; i <= sd.numMarkers; ++i)
        markers.emplace_back(markerThread, std::ref(sd), i);

    // 5. Give start signal
    {
        std::lock_guard<std::mutex> lk(sd.mtx);
        sd.startSignal = true;
    }
    sd.cvStart.notify_all();

    int activeMarkers = sd.numMarkers;

    // 6. Main loop
    while (activeMarkers > 0) {
        // 6.1 Wait until ALL active markers are blocked
        {
            std::unique_lock<std::mutex> lk(sd.mtx);
            sd.cvAllBlocked.wait(lk, [&]{
                return sd.blockedCount.load() >= activeMarkers;
            });
            sd.blockedCount.store(0); // reset for next round
        }

        // 6.2 Print array
        std::cout << "\n--- All markers blocked ---\n";
        printArray(sd);

        // 6.3 Ask which marker to terminate
        int choice = 0;
        while (true) {
            std::cout << "Enter marker id to terminate (1.." << sd.numMarkers << "): ";
            std::cin >> choice;
            if (choice >= 1 && choice <= sd.numMarkers &&
                !sd.terminateSignal[choice - 1])
                break;
            std::cout << "Invalid or already terminated marker. Try again.\n";
        }

        // 6.4 Send terminate signal to chosen marker
        {
            std::lock_guard<std::mutex> lk(sd.mtx);
            sd.terminateSignal[choice - 1] = true;
        }
        sd.cvMarker[choice - 1].notify_one();

        // 6.5 Wait for that marker to finish
        markers[choice - 1].join();
        --activeMarkers;

        // 6.6 Print array
        std::cout << "After termination of marker " << choice << ":\n";
        printArray(sd);

        // 6.7 Send continue signal to remaining active markers
        if (activeMarkers > 0) {
            std::lock_guard<std::mutex> lk(sd.mtx);
            for (int i = 0; i < sd.numMarkers; ++i) {
                if (!sd.terminateSignal[i]) {
                    sd.continueSignal[i] = true;
                    sd.cvMarker[i].notify_one();
                }
            }
        }
    }

    // 7. All markers finished
    std::cout << "\nAll markers terminated. Final array:\n";
    printArray(sd);
    return 0;
}
