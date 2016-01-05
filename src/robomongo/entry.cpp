#include <QApplication>
#include <QDesktopWidget>
#include <QDebug>

#include <locale.h>
#include <mongo/bson/bsonobjbuilder.h>
#include <robomongo/core/utils/BsonUtils.h>

using namespace Robomongo;

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    mongo::BSONObjBuilder m;
    m.append("test", 56);
    const mongo::BSONObj &obj = m.obj();

    std::string str = BsonUtils::jsonString(obj, mongo::TenGen, 1, DefaultEncoding, Utc);

    qDebug() << "Hello!";
    qDebug() << QString::fromStdString(str);

    return app.exec();
}

