#include "RoboCrypt.h"

#include <cmath>
#include <iostream>
#include <random>

#include <QDir>
#include <QFileInfo>
#include <QIODevice>
#include <QString>
#include <QTextStream>

namespace Robomongo {

    long long RoboCrypt::_KEY = 0;

    void RoboCrypt::initKey() 
    {
        const auto KEY_FILE = QString("%1/.3T/robo-3t/robo3t.bin").arg(QDir::homePath());
        QString fileContent;
        QFileInfo fileInfo(KEY_FILE);
        if (fileInfo.exists() && fileInfo.isFile()) {
            QFile keyFile(KEY_FILE);
            if (!keyFile.open(QIODevice::ReadOnly))
                throw std::runtime_error("Unable to load keyFile");

            QTextStream in(&keyFile);
            fileContent = in.readAll();    // todo: can be empty
            _KEY = fileContent.toLongLong();
        }
        else {
            std::random_device rd;
            std::mt19937_64 e2{ rd() };
            std::uniform_int_distribution<long long int> dist{ std::llround(std::pow(2,61)), std::llround(std::pow(2,62)) };
            _KEY = dist(e2);
            // 
            QFile file(KEY_FILE);
            //if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
            //    return false;
            file.open(QIODevice::WriteOnly | QIODevice::Text);
            QTextStream out(&file);
            out << QString::number(_KEY);
        }
    }

}