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

#include "camerabinfocus.h"
#include "camerabinsession.h"

#include <gst/interfaces/photography.h>

#include <QDebug>
#include <QtCore/qmetaobject.h>

//#define CAMERABIN_DEBUG 1
#define ZOOM_PROPERTY "zoom"
#define MAX_ZOOM_PROPERTY "max-zoom"

QT_BEGIN_NAMESPACE

CameraBinFocus::CameraBinFocus(CameraBinSession *session)
    :QCameraFocusControl(session),
     m_session(session),
     m_focusMode(QCameraFocus::AutoFocus),
     m_focusStatus(QCamera::Unlocked),
     m_focusZoneStatus(QCameraFocusZone::Selected)
{
    connect(m_session, SIGNAL(stateChanged(QCamera::State)),
            this, SLOT(_q_handleCameraStateChange(QCamera::State)));
}

CameraBinFocus::~CameraBinFocus()
{
}

QCameraFocus::FocusMode CameraBinFocus::focusMode() const
{
    return m_focusMode;
}

void CameraBinFocus::setFocusMode(QCameraFocus::FocusMode mode)
{
    if (isFocusModeSupported(mode)) {
        m_focusMode = mode;
    }
}

bool CameraBinFocus::isFocusModeSupported(QCameraFocus::FocusMode mode) const
{
    return mode & QCameraFocus::AutoFocus;
}

qreal CameraBinFocus::maximumOpticalZoom() const
{
    return 1.0;
}

qreal CameraBinFocus::maximumDigitalZoom() const
{
    gfloat zoomFactor = 1.0;
    g_object_get(GST_BIN(m_session->cameraBin()), MAX_ZOOM_PROPERTY, &zoomFactor, NULL);
    return zoomFactor;
}

qreal CameraBinFocus::opticalZoom() const
{
    return 1.0;
}

qreal CameraBinFocus::digitalZoom() const
{
    gfloat zoomFactor = 1.0;
    g_object_get(GST_BIN(m_session->cameraBin()), ZOOM_PROPERTY, &zoomFactor, NULL);
    return zoomFactor;
}

void CameraBinFocus::zoomTo(qreal optical, qreal digital)
{
    Q_UNUSED(optical);
    digital = qBound(qreal(1.0), digital, maximumDigitalZoom());
    g_object_set(GST_BIN(m_session->cameraBin()), ZOOM_PROPERTY, digital, NULL);
    emit digitalZoomChanged(digital);
}

QCameraFocus::FocusPointMode CameraBinFocus::focusPointMode() const
{
    return QCameraFocus::FocusPointAuto;
}

void CameraBinFocus::setFocusPointMode(QCameraFocus::FocusPointMode mode)
{
    Q_UNUSED(mode);
}

bool CameraBinFocus::isFocusPointModeSupported(QCameraFocus::FocusPointMode mode) const
{
    return mode == QCameraFocus::FocusPointAuto;
}

QPointF CameraBinFocus::customFocusPoint() const
{
    return QPointF(0.5, 0.5);
}

void CameraBinFocus::setCustomFocusPoint(const QPointF &point)
{
    Q_UNUSED(point);
}

QCameraFocusZoneList CameraBinFocus::focusZones() const
{
    return QCameraFocusZoneList() << QCameraFocusZone(QRectF(0.35, 0.35, 0.3, 0.3), m_focusZoneStatus);
}


void CameraBinFocus::handleFocusMessage(GstMessage *gm)
{
    //it's a sync message, so it's called from non main thread
    if (gst_structure_has_name(gm->structure, GST_PHOTOGRAPHY_AUTOFOCUS_DONE)) {
        gint status = GST_PHOTOGRAPHY_FOCUS_STATUS_NONE;
        gst_structure_get_int (gm->structure, "status", &status);
        QCamera::LockStatus focusStatus = m_focusStatus;
        QCamera::LockChangeReason reason = QCamera::UserRequest;

        switch (status) {
            case GST_PHOTOGRAPHY_FOCUS_STATUS_FAIL:
                focusStatus = QCamera::Unlocked;
                reason = QCamera::LockFailed;
                break;
            case GST_PHOTOGRAPHY_FOCUS_STATUS_SUCCESS:
                focusStatus = QCamera::Locked;
                break;
            case GST_PHOTOGRAPHY_FOCUS_STATUS_NONE:
                break;
            case GST_PHOTOGRAPHY_FOCUS_STATUS_RUNNING:
                focusStatus = QCamera::Searching;
                break;
            default:
                break;
        }

        static int signalIndex = metaObject()->indexOfSlot(
                    "_q_setFocusStatus(QCamera::LockStatus,QCamera::LockChangeReason)");
        metaObject()->method(signalIndex).invoke(this,
                                                 Qt::QueuedConnection,
                                                 Q_ARG(QCamera::LockStatus,focusStatus),
                                                 Q_ARG(QCamera::LockChangeReason,reason));
    }
}

void CameraBinFocus::_q_setFocusStatus(QCamera::LockStatus status, QCamera::LockChangeReason reason)
{
#ifdef CAMERABIN_DEBUG
    qDebug() << Q_FUNC_INFO << "Current:"
                << m_focusStatus
                << "New:"
                << status << reason;
#endif

    if (m_focusStatus != status) {
        m_focusStatus = status;

        QCameraFocusZone::FocusZoneStatus zonesStatus =
                m_focusStatus == QCamera::Locked ?
                    QCameraFocusZone::Focused : QCameraFocusZone::Selected;

        if (m_focusZoneStatus != zonesStatus) {
            m_focusZoneStatus = zonesStatus;
            emit focusZonesChanged();
        }

        emit _q_focusStatusChanged(m_focusStatus, reason);
    }
}

void CameraBinFocus::_q_handleCameraStateChange(QCamera::State state)
{
    if (state != QCamera::ActiveState)
        _q_setFocusStatus(QCamera::Unlocked, QCamera::LockLost);
}

void CameraBinFocus::_q_startFocusing()
{
    _q_setFocusStatus(QCamera::Searching, QCamera::UserRequest);
    gst_photography_set_autofocus(m_session->photography(), TRUE);
}

void CameraBinFocus::_q_stopFocusing()
{
    gst_photography_set_autofocus(m_session->photography(), FALSE);
    _q_setFocusStatus(QCamera::Unlocked, QCamera::UserRequest);
}

QT_END_NAMESPACE
