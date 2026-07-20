#include "Order/PriceLevel.hpp"
#include <algorithm>

Price PriceLevel::getPrice() const {
    return price_;
}

Quantity PriceLevel::getTotalQuantity() const {
    return totalQty_;
}

int PriceLevel::count() const {
    return static_cast<int>(orders_.size());
}

bool PriceLevel::empty() const {
    return orders_.empty();
}

const std::list<OrderId>& PriceLevel::getOrderIds() const {
    return orders_;
}

OrderId PriceLevel::front_id() const {
    return orders_.front();
}

void PriceLevel::decrease_total_quantity(Quantity qty) {
    totalQty_ -= qty;
}

PriceLevel::Iterator PriceLevel::add(OrderId id, Quantity qty) {
    orders_.push_back(id);
    totalQty_ += qty;
    return std::prev(orders_.end());
}

void PriceLevel::pop_front(Quantity fillQty) {
    if (orders_.empty()) {
        return;
    }

    totalQty_ -= fillQty;
    orders_.pop_front();
}

PriceLevel::Iterator PriceLevel::erase(Iterator it, Quantity qty) {
    totalQty_ -= qty;
    return orders_.erase(it);
}

bool PriceLevel::remove(OrderId id, Quantity qty) {
    auto it = std::find(orders_.begin(), orders_.end(), id);
    if (it == orders_.end()) {
        return false;
    }

    orders_.erase(it);
    totalQty_ -= qty;
    return true;
}