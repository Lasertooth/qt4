/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/
#ifndef BASELINEPROTOCOL_H
#define BASELINEPROTOCOL_H

#include <QDataStream>
#include <QTcpSocket>
#include <QImage>
#include <QVector>

#define QLS QLatin1String
#define QLC QLatin1Char

#define FileFormat "png"

struct PlatformInfo
{
    PlatformInfo(bool useLocal = false);

    QString hostName;
    QString osName;
    int     osVersion;
    QString qtVersion;
    QString buildKey;
    QString gitCommit;
};
QDataStream & operator<< (QDataStream &stream, const PlatformInfo &p);
QDataStream & operator>> (QDataStream& stream, PlatformInfo& p);

struct ImageItem
{
public:
    ImageItem()
        : status(Ok), renderFormat(QImage::Format_Invalid), engine(Raster), scriptChecksum(0)
    {}
    ImageItem(const ImageItem &other)
    { *this = other; }
    ~ImageItem()
    {}
    ImageItem &operator=(const ImageItem &other);
    static quint64 computeChecksum(const QImage& image);
    QString engineAsString() const;
    QString formatAsString() const;

    enum ItemStatus {
        Ok = 0,
        BaselineNotFound = 1,
        IgnoreItem = 2
    };

    enum GraphicsEngine {
        Raster = 0,
        OpenGL = 1
    };

    QString scriptName;
    ItemStatus status;
    QImage::Format renderFormat;
    GraphicsEngine engine;
    QImage image;
    QList<quint64> imageChecksums;
    // tbd: add diffscore
    quint16 scriptChecksum;
};
QDataStream & operator<< (QDataStream &stream, const ImageItem &ii);
QDataStream & operator>> (QDataStream& stream, ImageItem& ii);

Q_DECLARE_METATYPE(ImageItem);

typedef QVector<ImageItem> ImageItemList;

class BaselineProtocol
{
public:
    BaselineProtocol() {}
    ~BaselineProtocol();

    // ****************************************************
    // Important constants here
    // ****************************************************
    enum Constant {
        ProtocolVersion = 1,
        ServerPort = 54129,
        Timeout = 10000
    };

    enum Command {
        UnknownError = 0,
        // Queries
        AcceptPlatformInfo = 1,
        RequestBaselineChecksums = 2,
        AcceptNewBaseline = 4,
        AcceptMismatch = 5,
        // Responses
        Ack = 128,
    };

    // For client:
    bool connect();
    bool requestBaselineChecksums(ImageItemList *itemList);
    bool submitNewBaseline(const ImageItem &item, QByteArray *serverMsg);
    bool submitMismatch(const ImageItem &item, QByteArray *serverMsg);

    // For server:
    bool acceptConnection(PlatformInfo *pi);

    QString errorMessage();

private:
    bool sendItem(Command cmd, const ImageItem &item);

    bool sendBlock(Command cmd, const QByteArray &block);
    bool receiveBlock(Command *cmd, QByteArray *block);
    QString errMsg;
    QTcpSocket socket;

    friend class BaselineThread;
    friend class BaselineHandler;
};


#endif // BASELINEPROTOCOL_H
