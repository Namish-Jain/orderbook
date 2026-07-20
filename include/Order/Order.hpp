#pragma once

#include <chrono>
#include <string>
#include <cstdint>
#include "OrderType.hpp"

using OrderId = uint64_t;
using Price = int64_t;       // Fixed Point: actual price * PRICE_SCALE
using Quantity = uint64_t;
using Timestamp = uint64_t;  // In nanoseconds
constexpr int64_t PRICE_SCALE = 10000; 


class Order {

private:

    OrderId id;
    Price price;
    Quantity quantity;
    Timestamp timestamp;
    OrderType type;
    static inline int orderCounter = 0;

public:

    Order(Price price, Quantity quantity);
    virtual ~Order();
    virtual std::string getOrderClassType() const = 0;
    OrderId getOrderId() const;
    double priceToDouble(Price price);
    Price getPrice() const;
    Quantity getQuantity() const;
    Timestamp  getTimestamp() const;
    OrderType getOrderType() const;
    void setOrderType(OrderType type);
//  void reduceQuantity(Quantity amount); 

};