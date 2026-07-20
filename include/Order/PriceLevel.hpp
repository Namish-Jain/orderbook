#pragma once
#include <list>
#include "Order.hpp"

// Price Level holds all resting orders at a given price in a FIFO order via a linked list
class PriceLevel {

private:

    std::list<OrderId> orders_;
    Price price_; 
    Quantity totalQty_; // keep track of the total quantity at this price level

public:
    explicit PriceLevel(Price price) : price_(price), totalQty_(0) {}

    Price getPrice() const;

    Quantity getTotalQuantity() const;

    int count() const;

    bool empty() const;

    const std::list<OrderId>& getOrderIds() const;

    OrderId front_id() const;

    void decrease_total_quantity(Quantity qty);

    using Iterator = std::list<OrderId>::iterator;

    Iterator add(OrderId id, Quantity qty);

    // Pop the front order after it has been filled
    // Reduce the total quantity by the filled quantity (the quantity of the order that was at the front)
    void pop_front(Quantity fillQty);

    Iterator erase(Iterator it, Quantity qty);

    bool remove(OrderId id, Quantity qty);

};