#include "OrderFactory.hpp"
#include "LimitOrder.hpp"

class LimitOrderFactory : OrderFactory {

public:
    std::shared_ptr<Order> createOrder(Price price, Quantity quantity) override;

};