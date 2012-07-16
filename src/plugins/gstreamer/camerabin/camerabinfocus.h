/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef CAMERABINFOCUSCONTROL_H
#define CAMERABINFOCUSCONTROL_H

#include <qcamera.h>
#include <qcamerafocuscontrol.h>

#include <gst/gst.h>
#include <glib.h>

QT_BEGIN_NAMESPACE

class CameraBinSession;

class CameraBinFocus  : public QCameraFocusControl
{
    Q_OBJECT

public:
    CameraBinFocus(CameraBinSession *session);
    virtual ~CameraBinFocus();

    QCameraFocus::FocusMode focusMode() const;
    void setFocusMode(QCameraFocus::FocusMode mode);
    bool isFocusModeSupported(QCameraFocus::FocusMode mode) const;

    qreal maximumOpticalZoom() const;
    qreal maximumDigitalZoom() const;
    qreal opticalZoom() const;
    qreal digitalZoom() const;

    void zoomTo(qreal optical, qreal digital) ;

    QCameraFocus::FocusPointMode focusPointMode() const;
    void setFocusPointMode(QCameraFocus::FocusPointMode mode) ;
    bool isFocusPointModeSupported(QCameraFocus::FocusPointMode) const;
    QPointF customFocusPoint() const;
    void setCustomFocusPoint(const QPointF &point);

    QCameraFocusZoneList focusZones() const;

    void handleFocusMessage(GstMessage*);
    QCamera::LockStatus focusStatus() const { return m_focusStatus; }

Q_SIGNALS:
    void _q_focusStatusChanged(QCamera::LockStatus status, QCamera::LockChangeReason reason);

public Q_SLOTS:
    void _q_startFocusing();
    void _q_stopFocusing();

private Q_SLOTS:
    void _q_setFocusStatus(QCamera::LockStatus status, QCamera::LockChangeReason reason);
    void _q_handleCameraStateChange(QCamera::State state);

private:
    CameraBinSession *m_session;
    QCameraFocus::FocusMode m_focusMode;
    QCamera::LockStatus m_focusStatus;
    QCameraFocusZone::FocusZoneStatus m_focusZoneStatus;
};

QT_END_NAMESPACE

#endif // CAMERABINFOCUSCONTROL_H
