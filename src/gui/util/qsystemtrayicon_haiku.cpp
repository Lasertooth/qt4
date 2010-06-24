/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#include "qsystemtrayicon_p.h"

#ifndef QT_NO_SYSTEMTRAYICON

QT_BEGIN_NAMESPACE

#include "qsystemtrayicon.h"
#include "qdebug.h"
#include "qcolor.h"

#include <OS.h>
#include <Application.h>
#include <Window.h>
#include <Message.h>
#include <Deskbar.h>
#include <View.h>
#include <Roster.h>
#include <Screen.h>
#include <Resources.h>
#include <Bitmap.h>
#include <Looper.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define TRAY_MOUSEDOWN 	1
#define TRAY_MOUSEUP	2

#define DBAR_SIGNATURE 	"application/x-vnd.Be-TSKB"

QSystemTrayIconLooper::QSystemTrayIconLooper() : QObject(), BLooper("traylooper")
{	
}

thread_id 
QSystemTrayIconLooper::Run(void)
{
	thread_id Thread = BLooper::Run();	
	return Thread;
}

void 
QSystemTrayIconLooper::MessageReceived(BMessage* theMessage)
{
	if(theMessage->what == 'TRAY') {
		BMessage *mes = new BMessage(*theMessage);
		sendHaikuMessage(mes);
	}
	BLooper::MessageReceived(theMessage);
} 

QSystemTrayIconSys::QSystemTrayIconSys(QSystemTrayIcon *object)
    : ReplicantId(0), q(object), ignoreNextMouseRelease(false)
{
	ReplicantId = DeskBarLoadIcon();

	Looper = new QSystemTrayIconLooper();
	Looper->Run();

	BMessage mes('MSGR');
	QSystemTrayIconSys *sys=this;
	mes.AddMessenger("messenger",BMessenger(NULL,Looper));
	mes.AddData("qtrayobject",B_ANY_TYPE,&sys,sizeof(void*));

	SendMessageToReplicant(&mes);
		
	QObject::connect(Looper,SIGNAL(sendHaikuMessage(BMessage *)),this,SLOT(HaikuEvent(BMessage *)),Qt::QueuedConnection);
}

QSystemTrayIconSys::~QSystemTrayIconSys()
{
	BDeskbar deskbar;
	if(ReplicantId>0)
		deskbar.RemoveItem(ReplicantId);
	delete Looper;
}

void QSystemTrayIconSys::HaikuEvent(BMessage *m)
{	
	int32 event = 0;
	BPoint point(0,0);
	int32 buttons = 0,
		  clicks = 0;

	m->FindInt32("event",&event);
	m->FindPoint("point",&point);
	m->FindInt32("buttons",&buttons);
	m->FindInt32("clicks",&clicks);
	
	switch(event) {
		case TRAY_MOUSEUP:
			{				                
				if(buttons==B_PRIMARY_MOUSE_BUTTON) {
				if (ignoreNextMouseRelease)
                    ignoreNextMouseRelease = false;
                else
                    emit q->activated(QSystemTrayIcon::Trigger);
					break;
				}
				if(buttons==B_TERTIARY_MOUSE_BUTTON) {
					emit q->activated(QSystemTrayIcon::MiddleClick);
					break;
				}
				if(buttons==B_SECONDARY_MOUSE_BUTTON) {
					QPoint gpos = QPoint(point.x,point.y);
	                if (q->contextMenu()) {
	                    q->contextMenu()->popup(gpos);
	
						BScreen screen(NULL);
	                    QRect desktopRect( screen.Frame().left, screen.Frame().top,
	                    				   screen.Frame().right, screen.Frame().bottom);
	                    int maxY = desktopRect.y() + desktopRect.height() - q->contextMenu()->height();
	                    if (gpos.y() > maxY) {
	                        gpos.ry() = maxY;
	                        q->contextMenu()->move(gpos);
	                    }
	                }
	                emit q->activated(QSystemTrayIcon::Context);		
	             	break;   			
				}
			}
			break;
		case TRAY_MOUSEDOWN:
			{				
				if(buttons==B_PRIMARY_MOUSE_BUTTON && clicks==2) {
					ignoreNextMouseRelease = true;
					emit q->activated(QSystemTrayIcon::DoubleClick);
					break;
				}
			}
			break;
		default:
			break;
	}
}

void QSystemTrayIconSys::UpdateIcon()
{    
    QIcon qicon = q->icon();
    if (qicon.isNull())
        return;

    QSize size = qicon.actualSize(QSize(16, 16));
    QPixmap pm = qicon.pixmap(size);
    if (pm.isNull())
        return;
	
	BBitmap *icon = pm.toHaikuBitmap();
	if(icon) {
		BMessage	bits(B_ARCHIVED_OBJECT);
		icon->Archive(&bits);	
		BMessage *mes = new BMessage('BITS');
		mes->AddMessage("icon",&bits);
		bits.MakeEmpty();
		SendMessageToReplicant(mes);
		delete icon;
	}	
}

BMessenger 
QSystemTrayIconSys::GetShelfMessenger(void)
{
	BMessenger aResult;
	status_t aErr = B_OK;
	BMessenger aDeskbar(DBAR_SIGNATURE, -1, &aErr);
	if (aErr != B_OK)return aResult;

	BMessage aMessage(B_GET_PROPERTY);
	
	aMessage.AddSpecifier("Messenger");
	aMessage.AddSpecifier("Shelf");
	aMessage.AddSpecifier("View", "Status");
	aMessage.AddSpecifier("Window", "Deskbar");
	
	BMessage aReply;

	if (aDeskbar.SendMessage(&aMessage, &aReply, 1000000, 1000000) == B_OK)
		aReply.FindMessenger("result", &aResult);
	return aResult;
}


status_t 
QSystemTrayIconSys::SendMessageToReplicant(BMessage *msg)
{
	if(ReplicantId<=0)
		return B_ERROR;
		
	BMessage aReply;
	status_t aErr = B_OK;
	
	msg->AddInt32( "what2", msg->what );
	msg->what = B_SET_PROPERTY;

	BMessage	uid_specifier(B_ID_SPECIFIER);
	
	msg->AddSpecifier("View");
	uid_specifier.AddInt32("id", ReplicantId);
	uid_specifier.AddString("property", "Replicant");
	msg->AddSpecifier(&uid_specifier);
		
	aErr = GetShelfMessenger().SendMessage( msg, (BHandler*)NULL, 1000000 );
	return aErr;
}

int32	
QSystemTrayIconSys::ExecuteCommand(char *command)
{
   FILE *fpipe;
   char line[256];
   if ( !(fpipe = (FILE*)popen(command,"r")) )
   		return -1;

   fgets( line, sizeof line, fpipe);
   pclose(fpipe);
   
   int res = atoi(line);
   return res;
}

int32 
QSystemTrayIconSys::DeskBarLoadIcon(team_id tid)
{
	char cmd[256];
	sprintf(cmd,"qsystray %d",tid);	
	int32 id = ExecuteCommand(cmd);
	return id;
}

int32 
QSystemTrayIconSys::DeskBarLoadIcon(void)
{
	thread_info threadInfo;
	status_t error = get_thread_info(find_thread(NULL), &threadInfo);
	if (error != B_OK) {
		fprintf(stderr, "Failed to get info for the current thread: %s\n",
			strerror(error));
			return -1;	
	}
	team_id sTeam = threadInfo.team;
	
	return DeskBarLoadIcon(sTeam);
}

void QSystemTrayIconPrivate::install_sys()
{
	fprintf(stderr, "Reimplemented: QSystemTrayIconPrivate::install_sys \n");
    Q_Q(QSystemTrayIcon);
    if (!sys) {
        sys = new QSystemTrayIconSys(q);		
        sys->UpdateIcon();
    }
}

void QSystemTrayIconPrivate::showMessage_sys(const QString &title,  const QString &message, QSystemTrayIcon::MessageIcon type, int timeOut)
{
	Q_UNUSED(title);
	Q_UNUSED(message);
	Q_UNUSED(type);
	Q_UNUSED(timeOut);
	fprintf(stderr, "Unimplemented:  QSystemTrayIconPrivate::showMessage_sys\n");	
}

QRect QSystemTrayIconPrivate::geometry_sys() const
{
	fprintf(stderr, "Unimplemented: QSystemTrayIconPrivate::geometry_sys \n");
	return QRect();
}

void QSystemTrayIconPrivate::remove_sys()
{
	fprintf(stderr, "Reimplemented: QSystemTrayIconPrivate::remove_sys \n");
	if(sys) {    
    	delete sys;
    	sys = NULL;
	}
}

void QSystemTrayIconPrivate::updateIcon_sys()
{
	fprintf(stderr, "Reimplemented:  QSystemTrayIconPrivate::updateIcon_sys\n");	
    if (sys) {
	    sys->UpdateIcon();
    }
}

void QSystemTrayIconPrivate::updateMenu_sys()
{
	fprintf(stderr, "Unimplemented:  QSystemTrayIconPrivate::updateMenu_sys\n");
}

void QSystemTrayIconPrivate::updateToolTip_sys()
{
	fprintf(stderr, "Unimplemented:  QSystemTrayIconPrivate::updateToolTip_sys\n");
}

bool QSystemTrayIconPrivate::isSystemTrayAvailable_sys()
{
	fprintf(stderr, "Reimplemented:  QSystemTrayIconPrivate::isSystemTrayAvailable_sys\n");
	return true;
}

bool QSystemTrayIconPrivate::supportsMessages_sys()
{
    return false;
}

QT_END_NAMESPACE

#endif // QT_NO_SYSTEMTRAYICON
