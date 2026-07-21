// benchmark.cpp
//
// Measures per-order processing latency and overall throughput of the
// OrderBook matching engine (lobV3 architecture: OrderBook + LimitOrder).
//
// Build (add to CMakeLists.txt):
//   add_executable(benchmark
//       benchmark.cpp
//       src/OrderBook/OrderBook.cpp
//       src/Order/Order.cpp
//       src/Order/LimitOrder.cpp
//       src/Order/PriceLevel.cpp
//       src/OrderBook/PriceTimeOrderMatchingStrategy.cpp
//   )
//   target_include_directories(benchmark PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
//
// Run:
//   ./benchmark            (defaults to 100,000 orders)
//   ./benchmark 1000000    (custom order count)

#include "OrderBook/OrderBook.hpp"
#include "Order/LimitOrder.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <random>
#include <vector>

static Price toFixedPrice(double value) {
    return static_cast<Price>(std::llround(value * PRICE_SCALE));
}

int main(int argc, char** argv) {
    const size_t N = (argc > 1) ? static_cast<size_t>(std::atol(argv[1])) : 100000;

    OrderBook book;

    std::mt19937 rng(42);
    std::uniform_real_distribution<double> price_offset(-0.50, 0.50); // +/- 50 cents around mid
    std::uniform_int_distribution<int> qty_dist(1, 200);
    std::uniform_int_distribution<int> side_dist(0, 1);

    const double mid = 150.00;

    std::vector<long long> latencies_ns;
    latencies_ns.reserve(N);

    const auto run_start = std::chrono::high_resolution_clock::now();

    for (size_t i = 0; i < N; ++i) {
        const OrderType side = side_dist(rng) == 0 ? OrderType::BUY : OrderType::SELL;
        const double px = mid + price_offset(rng);
        const Quantity qty = static_cast<Quantity>(qty_dist(rng));
        const Price fixedPrice = toFixedPrice(px);

        LimitOrder order(fixedPrice, qty, fixedPrice);
        order.setOrderType(side);

        const auto t0 = std::chrono::high_resolution_clock::now();
        book.addOrder(order);
        const auto t1 = std::chrono::high_resolution_clock::now();

        latencies_ns.push_back(
            std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());
    }

    const auto run_end = std::chrono::high_resolution_clock::now();
    const double total_seconds =
        std::chrono::duration<double>(run_end - run_start).count();

    std::sort(latencies_ns.begin(), latencies_ns.end());

    auto percentile = [&](double p) -> long long {
        size_t idx = static_cast<size_t>(p * static_cast<double>(latencies_ns.size() - 1));
        return latencies_ns[idx];
    };

    double sum = 0.0;
    for (long long v : latencies_ns) sum += static_cast<double>(v);
    const double mean_ns = sum / static_cast<double>(latencies_ns.size());
    const double throughput = static_cast<double>(N) / total_seconds;

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "\n=== OrderBook Benchmark (lobV3) ===\n";
    std::cout << "Orders processed : " << N << "\n";
    std::cout << "Total time       : " << total_seconds << " s\n";
    std::cout << "Throughput       : " << throughput << " orders/sec\n\n";
    std::cout << "Latency (ns):\n";
    std::cout << "  mean : " << mean_ns << "\n";
    std::cout << "  p50  : " << percentile(0.50) << "\n";
    std::cout << "  p95  : " << percentile(0.95) << "\n";
    std::cout << "  p99  : " << percentile(0.99) << "\n";
    std::cout << "  max  : " << latencies_ns.back() << "\n\n";

    std::cout << "Book empty at end: " << (book.isEmpty() ? "yes" : "no") << "\n";

    return 0;
}