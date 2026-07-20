#include "OrderBook/OrderMatchingStrategy.hpp"
#include "Order/Order.hpp"

class PriceTimeOrderMatchingStrategy : public OrderMatchingStrategy {

public: 
    void matchOrders() override;

};