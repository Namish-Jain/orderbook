#include <memory>
#include "Order.hpp"
#include "OrderType.hpp"

class OrderFactory {

public:

    virtual std::shared_ptr<Order> createOrder(Price price, Quantity quantity) = 0;
    virtual ~OrderFactory();

};