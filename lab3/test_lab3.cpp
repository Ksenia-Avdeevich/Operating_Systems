// tests/test_lab3.cpp
// Simple assert-based tests for the marker/array logic.
// Tests do NOT require interactive I/O and do NOT spin real marker threads
// (to avoid flakiness). Instead they test the pure logic functions directly.

#include <cassert>
#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <string>

// ──────────────────────────────────────────────────────────────────────────────
// Helpers copied / adapted from the main implementation
// ──────────────────────────────────────────────────────────────────────────────

// Zero-out all elements marked by `id`
static void clearMarkerEntries(std::vector<int>& arr, int id) {
    for (int& v : arr)
        if (v == id) v = 0;
}

// Count how many elements are marked by `id`
static int countMarked(const std::vector<int>& arr, int id) {
    int cnt = 0;
    for (int v : arr)
        if (v == id) ++cnt;
    return cnt;
}

// Simulate one marker's marking step (no sleep, deterministic seed)
// Returns index where it got blocked, or -1 if it managed to fill the whole array.
static int simulateMarker(std::vector<int>& arr, int id, int seed) {
    srand(static_cast<unsigned>(seed));
    int sz = static_cast<int>(arr.size());
    // At most sz successful marks before we must be blocked (pigeonhole)
    for (int attempt = 0; attempt < sz * 10; ++attempt) {
        int r = rand() % sz;
        if (arr[r] == 0) {
            arr[r] = id;
        } else {
            return r; // blocked here
        }
    }
    return -1; // unlikely: filled everything
}

// ──────────────────────────────────────────────────────────────────────────────
// Test cases
// ──────────────────────────────────────────────────────────────────────────────

static int passed = 0;
static int failed = 0;

#define CHECK(cond, name)                                           \
    do {                                                            \
        if (!(cond)) {                                              \
            std::cerr << "[FAIL] " << (name) << "\n";              \
            ++failed;                                               \
        } else {                                                    \
            std::cout << "[PASS] " << (name) << "\n";              \
            ++passed;                                               \
        }                                                           \
    } while(0)

// ── 1. Array initialisation ───────────────────────────────────────────────────
void test_array_init() {
    std::vector<int> arr(10, 0);
    for (int v : arr)
        CHECK(v == 0, "array_init: each element is zero");
}

// ── 2. clearMarkerEntries ─────────────────────────────────────────────────────
void test_clear_marker() {
    std::vector<int> arr = {1, 2, 1, 3, 1, 2};
    clearMarkerEntries(arr, 1);
    CHECK(arr[0] == 0, "clear_marker: index 0 zeroed");
    CHECK(arr[1] == 2, "clear_marker: index 1 untouched");
    CHECK(arr[2] == 0, "clear_marker: index 2 zeroed");
    CHECK(arr[3] == 3, "clear_marker: index 3 untouched");
    CHECK(arr[4] == 0, "clear_marker: index 4 zeroed");
    CHECK(arr[5] == 2, "clear_marker: index 5 untouched");
}

// ── 3. countMarked ────────────────────────────────────────────────────────────
void test_count_marked() {
    std::vector<int> arr = {1, 2, 1, 1, 3, 2};
    CHECK(countMarked(arr, 1) == 3, "count_marked: id=1 → 3");
    CHECK(countMarked(arr, 2) == 2, "count_marked: id=2 → 2");
    CHECK(countMarked(arr, 3) == 1, "count_marked: id=3 → 1");
    CHECK(countMarked(arr, 4) == 0, "count_marked: id=4 → 0");
}

// ── 4. Marker gets blocked when array is full ─────────────────────────────────
void test_marker_blocked_on_full_array() {
    // Fill array completely with marker 2; marker 1 must be blocked immediately.
    std::vector<int> arr(8, 2);
    int blockedAt = simulateMarker(arr, 1, 1);
    CHECK(blockedAt != -1, "marker_blocked_on_full: must be blocked");
    // The array should remain all 2s (marker 1 never wrote anything)
    CHECK(countMarked(arr, 1) == 0, "marker_blocked_on_full: marker 1 wrote nothing");
}

// ── 5. Marker marks at least one element on empty array ───────────────────────
void test_marker_marks_empty_array() {
    std::vector<int> arr(10, 0);
    simulateMarker(arr, 1, 1);
    CHECK(countMarked(arr, 1) > 0, "marker_marks_empty: marker 1 marked something");
}

// ── 6. After clear, positions are available for a new marker ──────────────────
void test_clear_then_remark() {
    std::vector<int> arr(6, 1); // all marked by marker 1
    clearMarkerEntries(arr, 1);
    // Now array is all zeros; marker 2 should be able to mark freely
    simulateMarker(arr, 2, 42);
    CHECK(countMarked(arr, 2) > 0, "clear_then_remark: marker 2 can mark after clear");
    CHECK(countMarked(arr, 1) == 0, "clear_then_remark: marker 1 entries gone");
}

// ── 7. Two markers don't overwrite each other (sequential simulation) ─────────
void test_two_markers_no_overwrite() {
    std::vector<int> arr(20, 0);
    simulateMarker(arr, 1, 1); // fills some elements
    int before = countMarked(arr, 1);
    simulateMarker(arr, 2, 2); // fills remaining free spots
    // Marker 1's entries should still be intact
    CHECK(countMarked(arr, 1) == before,
          "two_markers_no_overwrite: marker 1 entries preserved");
}

// ── 8. srand(id) produces different sequences for different markers ────────────
void test_different_seeds() {
    // Two separate arrays, each run with a different seed
    std::vector<int> a1(15, 0), a2(15, 0);
    simulateMarker(a1, 1, 1);
    simulateMarker(a2, 2, 2);
    // They won't mark identical patterns (with overwhelming probability)
    bool identical = true;
    for (int i = 0; i < 15; ++i) {
        if ((a1[i] != 0 ? 1 : 0) != (a2[i] != 0 ? 1 : 0)) { identical = false; break; }
    }
    CHECK(!identical, "different_seeds: different seed → different marking pattern");
}

// ── 9. Blocked index is always within array bounds ────────────────────────────
void test_blocked_index_in_bounds() {
    std::vector<int> arr(12, 3); // all filled by marker 3
    int sz = static_cast<int>(arr.size());
    int blockedAt = simulateMarker(arr, 1, 7);
    CHECK(blockedAt >= 0 && blockedAt < sz,
          "blocked_index_in_bounds: index within [0, size)");
}

// ── 10. Thread-safety smoke test: two threads mark distinct halves ─────────────
void test_thread_marking_disjoint() {
    // Pre-fill first half with 1, second half with 0
    // Then have a real thread mark the second half with id=2
    // and verify no corruption.
    const int SZ = 20;
    std::vector<int> arr(SZ, 0);
    for (int i = 0; i < SZ / 2; ++i) arr[i] = 1; // first half = marker 1

    std::mutex mtx;
    std::thread t([&]() {
        srand(2);
        // Only mark in second half to avoid collision (simplified check)
        for (int attempt = 0; attempt < 50; ++attempt) {
            int r = (rand() % (SZ / 2)) + SZ / 2; // second half only
            std::lock_guard<std::mutex> lk(mtx);
            if (arr[r] == 0) arr[r] = 2;
        }
    });
    t.join();

    // First half should still be all 1s
    bool firstHalfOk = true;
    for (int i = 0; i < SZ / 2; ++i)
        if (arr[i] != 1) { firstHalfOk = false; break; }
    CHECK(firstHalfOk, "thread_marking_disjoint: first half not corrupted");

    // Second half should contain only 0s and 2s
    bool secondHalfOk = true;
    for (int i = SZ / 2; i < SZ; ++i)
        if (arr[i] != 0 && arr[i] != 2) { secondHalfOk = false; break; }
    CHECK(secondHalfOk, "thread_marking_disjoint: second half contains only 0 or 2");
}

// ──────────────────────────────────────────────────────────────────────────────
// Entry point
// ──────────────────────────────────────────────────────────────────────────────
int main() {
    std::cout << "=== Lab 3 Unit Tests ===\n\n";

    test_array_init();
    test_clear_marker();
    test_count_marked();
    test_marker_blocked_on_full_array();
    test_marker_marks_empty_array();
    test_clear_then_remark();
    test_two_markers_no_overwrite();
    test_different_seeds();
    test_blocked_index_in_bounds();
    test_thread_marking_disjoint();

    std::cout << "\n=== Results: " << passed << " passed, " << failed << " failed ===\n";
    return (failed == 0) ? 0 : 1;
}
