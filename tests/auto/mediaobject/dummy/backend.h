/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef PHONON_DUMMY_BACKEND_H
#define PHONON_DUMMY_BACKEND_H

#include <phonon/objectdescription.h>
#include <phonon/backendinterface.h>

#include <phonon/medianode.h>

#include <QtCore/QList>
#include <QtCore/QPointer>
#include <QtCore/QStringList>
#include <QtCore/QTimer>

QT_BEGIN_NAMESPACE

namespace Phonon
{
namespace Dummy
{
class AudioOutput;
class MediaObject;

class Backend : public QObject, public BackendInterface
{
    Q_OBJECT
    Q_INTERFACES(Phonon::BackendInterface)

public:
    Backend(QObject *parent = 0, const QVariantList & = QVariantList());
    virtual ~Backend();

    QObject *createObject(BackendInterface::Class, QObject *parent, const QList<QVariant> &args);

    QStringList availableMimeTypes() const;

    QList<int> objectDescriptionIndexes(ObjectDescriptionType type) const;
    QHash<QByteArray, QVariant> objectDescriptionProperties(ObjectDescriptionType type, int index) const;

    bool startConnectionChange(QSet<QObject *>);
    bool connectNodes(QObject *, QObject *);
    bool disconnectNodes(QObject *, QObject *);
    bool endConnectionChange(QSet<QObject *>);

Q_SIGNALS:
    void objectDescriptionChanged(ObjectDescriptionType);

private:
    QList<QPointer<AudioOutput> > m_audioOutputs;
};
}
} // namespace Phonon::Dummy

QT_END_NAMESPACE

#endif // PHONON_DUMMY_BACKEND_H
