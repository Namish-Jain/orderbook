#include "Order.hpp"

class LimitOrder : public Order {

private:

    Price limit;

public:

    LimitOrder(Price price, Quantity quantity, Price limit) :
    Order(price, quantity), limit(limit) {}

    std::string getOrderClassType() const override;
    Price getLimit() const;
    void setLimit(Price limit);

};