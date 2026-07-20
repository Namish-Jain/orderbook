#include "../../include/Order/Order.hpp"

Order::Order(Price price, Quantity quantity) : price(price), quantity(quantity) {
    id = ++orderCounter;
}

Order::~Order() = default;

OrderId Order::getOrderId() const {
    return id;
}

Price Order::getPrice() const {
    return price;
}

double Order::priceToDouble(Price price) {
    return static_cast<double>(price) / static_cast<double>(PRICE_SCALE);
}

Quantity Order::getQuantity() const {
    return quantity;
}

Timestamp Order::getTimestamp() const {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()
    ).count();
}

OrderType Order::getOrderType() const {
    return type;
}

void Order::setOrderType(OrderType type) {
    this -> type = type;
}