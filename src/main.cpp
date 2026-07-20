#include <cctype>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "OrderBook/OrderBook.hpp"
#include "Order/LimitOrder.hpp"

struct OpenOrder {
    OrderId id;
    OrderType type;
    Price price;
    Quantity quantity;
    bool active;
};

static OpenOrder makeOpenOrder(OrderId id, OrderType type, Price price, Quantity quantity, bool active) {
    return OpenOrder{id, type, price, quantity, active};
}

static Price toFixedPrice(double value) {
    return static_cast<Price>(std::llround(value * PRICE_SCALE));
}

static double toDoublePrice(Price value) {
    return static_cast<double>(value) / static_cast<double>(PRICE_SCALE);
}

int main() {
    OrderBook book;
    std::map<OrderId, OpenOrder> openOrders;
    double balance = 1000.0;

    auto printMenu = []() {
        std::cout << "\n\n=== Trading Menu ===\n";
        std::cout << "1. Show Balance\n";
        std::cout << "2. Show OrderBook\n";
        std::cout << "3. Add Bid / Ask\n";
        std::cout << "4. Modify Bid / Ask\n";
        std::cout << "5. Cancel Bid / Ask\n";
        std::cout << "6. Exit\n\n";
        std::cout << "Enter Your Choice: ";
    };

    auto printBalance = [&]() {
        std::cout << "Current balance: $" << std::fixed << std::setprecision(2) << balance << "\n";
    };

    auto addInitialOrders = [&]() {
        const std::vector<std::pair<Price, Quantity>> bidLevels = {
            {toFixedPrice(100.00), 10},
            {toFixedPrice(99.50), 8},
            {toFixedPrice(99.00), 6},
            {toFixedPrice(98.50), 5},
            {toFixedPrice(98.00), 7}
        };

        const std::vector<std::pair<Price, Quantity>> askLevels = {
            {toFixedPrice(100.50), 9},
            {toFixedPrice(101.00), 4},
            {toFixedPrice(101.50), 6},
            {toFixedPrice(102.00), 5},
            {toFixedPrice(102.50), 7}
        };

        for (const auto& [price, qty] : bidLevels) {
            LimitOrder order(price, qty, price);
            order.setOrderType(OrderType::BUY);
            book.addOrder(order);
            auto id = order.getOrderId();
            openOrders[id] = makeOpenOrder(id, OrderType::BUY, price, qty, true);
        }

        for (const auto& [price, qty] : askLevels) {
            LimitOrder order(price, qty, price);
            order.setOrderType(OrderType::SELL);
            book.addOrder(order);
            auto id = order.getOrderId();
            openOrders[id] = makeOpenOrder(id, OrderType::SELL, price, qty, true);
        }
    };

    auto findCrossingOrder = [&](const OpenOrder& incoming) -> std::optional<OrderId> {
        for (auto& [id, existing] : openOrders) {
            if (!existing.active || existing.type == incoming.type) {
                continue;
            }

            const bool crosses = incoming.type == OrderType::BUY
                ? existing.price <= incoming.price
                : existing.price >= incoming.price;

            if (crosses) {
                return id;
            }
        }
        return std::nullopt;
    };

    addInitialOrders();

    std::cout << "Initial OrderBook:\n";
    book.printOrderBook();
    std::cout << "\n";
    printBalance();

    while (true) {
        printMenu();
        int choice = 0;
        if (!(std::cin >> choice)) {
            std::cout << "Invalid input. Exiting.\n";
            break;
        }

        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        if (choice == 6) {
            break;
        }

        if (choice == 1) {
            std::cout << "\n";
            printBalance();
            continue;
        }

        if (choice == 2) {
            book.printOrderBook();
            continue;
        }

        if (choice == 3) {
            std::cout << "Enter side (bid/ask): ";
            std::string side;
            std::getline(std::cin, side);

            std::string normalizedSide;
            normalizedSide.reserve(side.size());
            for (char ch : side) {
                if (!std::isspace(static_cast<unsigned char>(ch))) {
                    normalizedSide.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
                }
            }

            if (normalizedSide != "bid" && normalizedSide != "ask") {
                std::cout << "Error: you can only enter 'bid' or 'ask'.\n";
                continue;
            }

            side = normalizedSide;

            std::cout << "Enter price: ";
            double priceInput = 0.0;
            std::cin >> priceInput;
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

            std::cout << "Enter quantity: ";
            Quantity qtyInput = 0;
            std::cin >> qtyInput;
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

            if (qtyInput == 0) {
                std::cout << "Quantity must be greater than zero.\n";
                continue;
            }

            const bool isBuy = (side == "bid" || side == "Bid" || side == "B" || side == "b");
            const OrderType orderType = isBuy ? OrderType::BUY : OrderType::SELL;
            const Price fixedPrice = toFixedPrice(priceInput);

            if (isBuy && balance < priceInput * static_cast<double>(qtyInput)) {
                std::cout << "Insufficient balance for this buy order.\n";
                continue;
            }

            LimitOrder newOrder(fixedPrice, qtyInput, fixedPrice);
            newOrder.setOrderType(orderType);

            auto incomingId = newOrder.getOrderId();
            auto crossingId = findCrossingOrder({incomingId, orderType, fixedPrice, qtyInput, true});

            Quantity filledQty = 0;
            if (crossingId.has_value()) {
                auto& restingOrder = openOrders[*crossingId];
                filledQty = std::min(qtyInput, restingOrder.quantity);
                if (filledQty > 0) {
                    if (orderType == OrderType::BUY) {
                        balance -= static_cast<double>(filledQty) * toDoublePrice(restingOrder.price);
                    } else {
                        balance += static_cast<double>(filledQty) * toDoublePrice(restingOrder.price);
                    }

                    restingOrder.quantity -= filledQty;
                    if (restingOrder.quantity == 0) {
                        restingOrder.active = false;
                        book.cancelOrder(*crossingId);
                    } else {
                        book.modifyOrder(*crossingId, toDoublePrice(restingOrder.price), restingOrder.quantity);
                    }

                    std::cout << "Trade matched for " << filledQty << " units at $"
                              << std::fixed << std::setprecision(2)
                              << toDoublePrice(restingOrder.price) << "\n";
                }
            }

            Quantity remainingQty = qtyInput - filledQty;
            if (remainingQty > 0) {
                LimitOrder restingOrder(fixedPrice, remainingQty, fixedPrice);
                restingOrder.setOrderType(orderType);
                book.addOrder(restingOrder);
                auto addedId = restingOrder.getOrderId();
                openOrders[addedId] = {addedId, orderType, fixedPrice, remainingQty, true};
            }

            if (filledQty == qtyInput) {
                std::cout << "Order fully matched and removed.\n";
            } else if (remainingQty > 0) {
                std::cout << "Order partially matched and the remainder is resting in the book.\n";
            }

            continue;
        }

        if (choice == 4) {
            std::cout << "Enter order id to modify: ";
            OrderId orderId = 0;
            std::cin >> orderId;
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

            auto it = openOrders.find(orderId);
            if (it == openOrders.end() || !it->second.active) {
                std::cout << "Order not found.\n";
                continue;
            }

            std::cout << "Enter new price: ";
            double newPrice = 0.0;
            std::cin >> newPrice;
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

            std::cout << "Enter new quantity: ";
            Quantity newQty = 0;
            std::cin >> newQty;
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

            if (newQty == 0) {
                std::cout << "Quantity must be greater than zero.\n";
                continue;
            }

            if (it->second.type == OrderType::BUY && balance < newPrice * static_cast<double>(newQty)) {
                std::cout << "Insufficient balance for this buy modification.\n";
                continue;
            }

            book.modifyOrder(orderId, newPrice, newQty);
            it->second.price = toFixedPrice(newPrice);
            it->second.quantity = newQty;
            std::cout << "Order modified.\n";
            continue;
        }

        if (choice == 5) {
            std::cout << "Enter order id to cancel: ";
            OrderId orderId = 0;
            std::cin >> orderId;
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

            if (book.cancelOrder(orderId)) {
                auto it = openOrders.find(orderId);
                if (it != openOrders.end()) {
                    it->second.active = false;
                }
                std::cout << "Order cancelled.\n";
            } else {
                std::cout << "Order not found.\n";
            }
            continue;
        }

        std::cout << "Invalid choice. Please try again.\n";
    }

    return 0;
}
