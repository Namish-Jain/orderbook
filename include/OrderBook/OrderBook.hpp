#pragma once
#include "Order/OrderType.hpp"
#include "Order/Order.hpp"
#include "Order/PriceLevel.hpp"
#include "OrderBook/OrderMatchingStrategy.hpp"
#include <map>
#include <unordered_map>
#include <string>

class OrderBook {

private:

    struct OrderEntry {
        Price price;
        Quantity quantity;
        OrderType type;
    };

    std::map<Price, PriceLevel, std::greater<Price>> bids_;
    std::map<Price, PriceLevel> asks_;
    OrderMatchingStrategy* strategy = nullptr;
    std::unordered_map<OrderId, OrderEntry> orders_;

    void addOrderToBook(OrderId id, const OrderEntry& entry);
    void removeOrderFromBook(OrderId id, const OrderEntry& entry);
    void matchAgainstRestingOrders(OrderId id, OrderEntry& entry);

public: 

    OrderBook();

    void setStrategy(OrderMatchingStrategy* strat);

    void addOrder(const Order& o);

    void matchOrder(Order& o);

    bool cancelOrder(OrderId id);

    bool modifyOrder(OrderId id, double newPrice, Quantity newQuantity);

    bool isEmpty() const;

    void print(int depth = 5) const;
    void printOrderBook() const;

};