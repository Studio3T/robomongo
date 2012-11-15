#include "Hello.h"
#include <QSharedData>

class HelloData : public QSharedData {
public:
};

Hello::Hello() : data(new HelloData)
{
}

Hello::Hello(const Hello &rhs) : data(rhs.data)
{
}

Hello &Hello::operator=(const Hello &rhs)
{
    if (this != &rhs)
        data.operator=(rhs.data);
    return *this;
}

Hello::~Hello()
{
}
