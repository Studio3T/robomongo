#ifndef HELLO_H
#define HELLO_H

#include <QSharedDataPointer>

class HelloData;

class Hello
{
public:
    Hello();
    Hello(const Hello &);
    Hello &operator=(const Hello &);
    ~Hello();
    
private:
    QSharedDataPointer<HelloData> data;
};

#endif // HELLO_H
