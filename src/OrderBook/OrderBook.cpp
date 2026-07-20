#include "OrderBook/OrderBook.hpp"
#include <cmath>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <utility>
#include <vector>

OrderBook::OrderBook() = default;

void OrderBook::setStrategy(OrderMatchingStrategy* strat) {
    strategy = strat;
}

void OrderBook::addOrderToBook(OrderId id, const OrderEntry& entry) {
    if (entry.type == OrderType::BUY) {
        auto [levelIt, inserted] = bids_.try_emplace(entry.price, entry.price);
        (void)inserted;
        levelIt->second.add(id, entry.quantity);
    } else {
        auto [levelIt, inserted] = asks_.try_emplace(entry.price, entry.price);
        (void)inserted;
        levelIt->second.add(id, entry.quantity);
    }
}

void OrderBook::removeOrderFromBook(OrderId id, const OrderEntry& entry) {
    if (entry.type == OrderType::BUY) {
        auto levelIt = bids_.find(entry.price);
        if (levelIt == bids_.end()) {
            return;
        }

        if (levelIt->second.remove(id, entry.quantity)) {
            if (levelIt->second.empty()) {
                bids_.erase(levelIt);
            }
        }
    } else {
        auto levelIt = asks_.find(entry.price);
        if (levelIt == asks_.end()) {
            return;
        }

        if (levelIt->second.remove(id, entry.quantity)) {
            if (levelIt->second.empty()) {
                asks_.erase(levelIt);
            }
        }
    }
}

void OrderBook::matchAgainstRestingOrders(OrderId id, OrderEntry& entry) {
    while (entry.quantity > 0) {
        if (entry.type == OrderType::BUY) {
            if (asks_.empty()) {
                break;
            }

            auto bestLevelIt = asks_.begin();
            const Price bestPrice = bestLevelIt->first;
            if (entry.price < bestPrice) {
                break;
            }

            auto& restingLevel = bestLevelIt->second;
            if (restingLevel.empty()) {
                asks_.erase(bestLevelIt);
                continue;
            }

            const OrderId restingId = restingLevel.front_id();
            auto restingIt = orders_.find(restingId);
            if (restingIt == orders_.end()) {
                restingLevel.pop_front(0);
                if (restingLevel.empty()) {
                    asks_.erase(bestLevelIt);
                }
                continue;
            }

            OrderEntry& restingEntry = restingIt->second;
            const Quantity tradeQty = std::min(entry.quantity, restingEntry.quantity);

            entry.quantity -= tradeQty;
            restingEntry.quantity -= tradeQty;

            if (restingEntry.quantity == 0) {
                orders_.erase(restingIt);
                restingLevel.pop_front(tradeQty);
                if (restingLevel.empty()) {
                    asks_.erase(bestLevelIt);
                }
            } else {
                restingLevel.decrease_total_quantity(tradeQty);
            }
        } else {
            if (bids_.empty()) {
                break;
            }

            auto bestLevelIt = bids_.begin();
            const Price bestPrice = bestLevelIt->first;
            if (entry.price > bestPrice) {
                break;
            }

            auto& restingLevel = bestLevelIt->second;
            if (restingLevel.empty()) {
                bids_.erase(bestLevelIt);
                continue;
            }

            const OrderId restingId = restingLevel.front_id();
            auto restingIt = orders_.find(restingId);
            if (restingIt == orders_.end()) {
                restingLevel.pop_front(0);
                if (restingLevel.empty()) {
                    bids_.erase(bestLevelIt);
                }
                continue;
            }

            OrderEntry& restingEntry = restingIt->second;
            const Quantity tradeQty = std::min(entry.quantity, restingEntry.quantity);

            entry.quantity -= tradeQty;
            restingEntry.quantity -= tradeQty;

            if (restingEntry.quantity == 0) {
                orders_.erase(restingIt);
                restingLevel.pop_front(tradeQty);
                if (restingLevel.empty()) {
                    bids_.erase(bestLevelIt);
                }
            } else {
                restingLevel.decrease_total_quantity(tradeQty);
            }
        }
    }

    if (entry.quantity == 0) {
        orders_.erase(id);
    } else {
        addOrderToBook(id, entry);
    }
}

void OrderBook::addOrder(const Order& o) {
    auto id = o.getOrderId();
    auto& entry = orders_[id];
    entry.price = o.getPrice();
    entry.quantity = o.getQuantity();
    entry.type = o.getOrderType();

    if (strategy != nullptr) {
        strategy->matchOrders();
    }

    matchAgainstRestingOrders(id, entry);
}

void OrderBook::matchOrder(Order& o) {
    auto id = o.getOrderId();
    auto entryIt = orders_.find(id);
    if (entryIt == orders_.end()) {
        auto& entry = orders_[id];
        entry.price = o.getPrice();
        entry.quantity = o.getQuantity();
        entry.type = o.getOrderType();
        matchAgainstRestingOrders(id, entry);
        return;
    }

    auto& entry = entryIt->second;
    matchAgainstRestingOrders(id, entry);
}

bool OrderBook::cancelOrder(OrderId id) {
    auto it = orders_.find(id);
    if (it == orders_.end()) {
        return false;
    }

    const auto& entry = it->second;
    removeOrderFromBook(id, entry);
    orders_.erase(it);
    return true;
}

bool OrderBook::modifyOrder(OrderId id, double newPrice, Quantity newQuantity) {
    auto it = orders_.find(id);
    if (it == orders_.end()) {
        return false;
    }

    auto& entry = it->second;
    if (entry.quantity == 0) {
        return false;
    }

    removeOrderFromBook(id, entry);

    entry.price = static_cast<Price>(std::llround(newPrice * PRICE_SCALE));
    entry.quantity = newQuantity;

    if (entry.quantity > 0) {
        addOrderToBook(id, entry);
    } else {
        orders_.erase(it);
    }

    return true;
}

bool OrderBook::isEmpty() const {
    return bids_.empty() && asks_.empty();
}

void OrderBook::print(int depth) const {
    const int w_id = 10;
    const int w_side = 8;
    const int w_price = 12;
    const int w_qty = 10;

    auto fmt_price = [](Price p) -> std::string {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(4) << static_cast<double>(p) / static_cast<double>(PRICE_SCALE);
        return oss.str();
    };

    const std::string bar = "+------------+----------+------------+------------+";
    std::cout << "\n" << bar << "\n";
    std::cout << "| ID         | Side     | Price      | Qty        |\n";
    std::cout << bar << "\n";

    std::vector<std::pair<Price, const PriceLevel*>> ask_levels;
    int cnt = 0;
    for (const auto& [price, level] : asks_) {
        if (cnt++ >= depth) {
            break;
        }
        ask_levels.push_back({price, &level});
    }

    for (auto it = ask_levels.rbegin(); it != ask_levels.rend(); ++it) {
        const PriceLevel& level = *it->second;
        for (OrderId id : level.getOrderIds()) {
            const auto orderIt = orders_.find(id);
            if (orderIt == orders_.end()) {
                continue;
            }
            const auto& entry = orderIt->second;
            std::cout << "| " << std::left << std::setw(w_id) << id
                      << " | " << std::left << std::setw(w_side) << "ASK"
                      << " | " << std::left << std::setw(w_price) << fmt_price(it->first)
                      << " | " << std::left << std::setw(w_qty) << entry.quantity << " |\n";
        }
    }

    if (ask_levels.empty()) {
        std::cout << "| (empty)    |          |            |            |\n";
    }

    std::cout << bar << "\n";

    cnt = 0;
    for (const auto& [price, level] : bids_) {
        if (cnt++ >= depth) {
            break;
        }
        for (OrderId id : level.getOrderIds()) {
            const auto orderIt = orders_.find(id);
            if (orderIt == orders_.end()) {
                continue;
            }
            const auto& entry = orderIt->second;
            std::cout << "| " << std::left << std::setw(w_id) << id
                      << " | " << std::left << std::setw(w_side) << "BID"
                      << " | " << std::left << std::setw(w_price) << fmt_price(price)
                      << " | " << std::left << std::setw(w_qty) << entry.quantity << " |\n";
        }
    }

    if (bids_.empty()) {
        std::cout << "| (empty)    |          |            |            |\n";
    }

    std::cout << bar << "\n";
    std::cout << std::flush;
}

void OrderBook::printOrderBook() const {
    print(5);
}