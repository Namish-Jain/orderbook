#include <memory>
#include <map>
#include <list>
#include "Order/Order.hpp"

class OrderMatchingStrategy {

public:
    virtual void matchOrders() = 0;

};