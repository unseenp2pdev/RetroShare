/***************************************************************************
 * Copyright (C) 2017-2019 by Savoir-faire Linux                                *
 * Author: Anthony L�onard <anthony.leonard@savoirfairelinux.com>          *
 * Author: Andreas Traczyk <andreas.traczyk@savoirfairelinux.com>          *
 *                                                                         *
 * This program is free software; you can redistribute it and/or modify    *
 * it under the terms of the GNU General Public License as published by    *
 * the Free Software Foundation; either version 3 of the License, or       *
 * (at your option) any later version.                                     *
 *                                                                         *
 * This program is distributed in the hope that it will be useful,         *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License       *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 **************************************************************************/

#include "smartlistmodel.h"

// Qt
#include <QDateTime>
#include <QImage>
#include <QSize>
#include <QPainter>
#include <QtXml>
#include <QDomComment>

#include "ChatLobbyWidget.h"
#include "MainWindow.h"
#include "retroshare/rspeers.h"
#include "retroshare/rsstatus.h"
#include "gui/common/AvatarDefs.h"

#include "models/conversationmodel.h"
#include "util/HandleRichText.h"

#define IMAGE_PUBLIC          ":/chat/img/groundchat.png"               //copy from ChatLobbyWidget
#define IMAGE_PRIVATE         ":/chat/img/groundchat_private.png"       //copy from ChatLobbyWidget
#define IMAGE_UNSEEN          ":/app/images/unseen32.png"

// Client
SmartListModel::SmartListModel(const std::string& accId, QObject *parent, bool contactList)
    : QAbstractItemModel(parent),
    accId_(accId),
    contactList_(contactList)
{
    setAccount(accId_);
}
/*
 * <body>
 *      <style type="text/css" RSOptimized="v2">.S1{font-size:13pt;}.S0{color:#000000;}.S1{font-weight:400;}.
 *                  S1{font-family:'';}.S1{font-style:normal;}
 *      </style>
 *      <span>
 *              <span class="S1">
 *                       <span class="S0">hihi</span>
 *              </span>
 *      </span>
 * </body>
 */
/*
 * <rates>
      <rate>
            <from>AUD</from>
            <to>CAD</to>
            <conversion>1.0079</conversion>
      </rate>
      <rate>...</rate>
      ...
  </rates>
 */

static QString readMsgFromXml(const QString &historyMsg)
{
    QDomDocument doc;
    if (!doc.setContent(historyMsg)) return "Media";

    //QDomNodeList rates = doc.elementsByTagName("body");
    QDomElement body = doc.firstChildElement("body");
    QDomElement span1 = body.firstChildElement("span");
    QDomElement span2 = span1.firstChildElement("span");
    QDomElement span3 = span2.firstChildElement("span");

    return span3.text();
}

int SmartListModel::rowCount(const QModelIndex &parent) const
{

    int count = 0;
    std::vector<conversationInfo> list = rsMsgs->getConversationItemList();
    count = static_cast<int>(list.size());
    return count; // A valid QModelIndex returns 0 as no entry has sub-elements
}

int SmartListModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 1;
}

static QImage getCirclePhoto(const QImage original, int sizePhoto)
{
    QImage target(sizePhoto, sizePhoto, QImage::Format_ARGB32_Premultiplied);
    target.fill(Qt::transparent);

    QPainter painter(&target);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
    painter.setBrush(QBrush(Qt::white));
    auto scaledPhoto = original
            .scaled(sizePhoto, sizePhoto, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation)
            .convertToFormat(QImage::Format_ARGB32_Premultiplied);
    int margin = 0;
    if (scaledPhoto.width() > sizePhoto) {
        margin = (scaledPhoto.width() - sizePhoto) / 2;
    }
    painter.drawEllipse(0, 0, sizePhoto, sizePhoto);
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.drawImage(0, 0, scaledPhoto, margin, 0);
    return target;
}

QVariant SmartListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    if (rsMsgs)
    {
        //Get avatar for groupchat or contact item
        uint32_t conversationMode = rsMsgs->getConversationListMode();
        std::vector<conversationInfo> list; // = rsMsgs->getConversationItemList();
        if (conversationMode == CONVERSATION_MODE_WITHOUT_FILTER)
        {
            list = rsMsgs->getConversationItemList();
        }
        else if (conversationMode == CONVERSATION_MODE_WITH_SEARCH_FILTER)
        {
            list = rsMsgs->getSearchFilteredConversationItemList();
        }

        if (index.row() >= static_cast<int>(list.size())) return QVariant();
        conversationInfo chatItem = list.at(index.row());

        //STATUS FOR CONTACT
        RsPeerId peerId(chatItem.rsPeerIdStr);
        QString presenceForChat = "no-status"; //for groupchat
        if (chatItem.contactType != 0)
        {
            StatusInfo statusContactInfo;

            rsStatus->getStatus(peerId,statusContactInfo);
            switch (statusContactInfo.status)
            {
                case RS_STATUS_OFFLINE:
                    presenceForChat = "offline";
                    break;
                case RS_STATUS_INACTIVE:
                    presenceForChat = "idle";
                    break;
                case RS_STATUS_ONLINE:
                    presenceForChat = "online";
                    break;
                case RS_STATUS_AWAY:
                    presenceForChat = "away";
                    break;
                case RS_STATUS_BUSY:
                    presenceForChat = "busy";
                    break;
            }
        }

        QImage avatar(IMAGE_UNSEEN);    //default avatar for UnseenP2P
        if (chatItem.contactType == 0)      //if this is a group chat
        {
           if (chatItem.groupChatType == 0) //public group chat
           {
                avatar = QImage(IMAGE_PUBLIC);
           }
           else     //private group chat
           {
                avatar = QImage(IMAGE_PRIVATE);
           }
        }
        else
        {
            QPixmap bestAvatar;
            AvatarDefs::getAvatarFromSslId(peerId, bestAvatar);
            avatar = bestAvatar.toImage();
        }

        ////////////////////////////////////////////
        /////// GET DATETIME for last msg //////////
        ////////////////////////////////////////////
        //date time format for msg: Mon Jan 21 12:39:35 1970 or Sat Oct 26 10:13:09 2019
        //                         "Sat Nov 9 17:49:42 2019"
        // if msg in the day, choose the "12:39"
        // if msg in the 7 days, choose the 3 first character like "Mon"
        // if msg older than 7 days, choose "Jan 21"
        QString timedateForMsgResult;
        //QDateTime dateTime =  QDateTime::fromTime_t(chatItem.lastMsgDatetime);
        QDateTime dateTime =  QDateTime::fromSecsSinceEpoch(chatItem.lastMsgDatetime);
        QString timedateForMsg = dateTime.toString();
        qint64 secondsOfDatetime = dateTime.toSecsSinceEpoch();

        qint64 now = QDateTime::currentDateTime().toSecsSinceEpoch();
        // if msg in the day, choose the "12:39"
        if (now - secondsOfDatetime <  86400)       //secondsIn1Day  =  86400;
        {
            // (SystemLocale)	"11/9/19 5:49 PM"	QString
            QString sysLocal = dateTime.toString(Qt::SystemLocaleDate);
            timedateForMsgResult = sysLocal.mid(sysLocal.length() - 8, 8);
        }
        // if msg in the 7 days, choose the 3 first character like "Mon"
        else if (now - secondsOfDatetime < 604800)      //secondsIn7Days = 604800;
        {
            timedateForMsgResult = timedateForMsg.mid(0,3);
        }
        // if msg older than 7 days, choose "Jan 21"
        else
        {
            // (SystemLocale)	"11/9/19 5:49 PM"	QString --> get this one: 11/9/19
            QString sysLocal = dateTime.toString(Qt::SystemLocaleDate);
            timedateForMsgResult = sysLocal.mid(0,8);
            //timedateForMsgResult.append(timedateForMsg.mid(19,4));
        }

        //GET LAST MSG from html format
        QString lastMsgQstr = QString::fromStdString(chatItem.lastMessage);
        QDomDocument docCheck;
        QString temp = lastMsgQstr;
        QString lastMsg;
        if (chatItem.contactType == 0)
        {
            if (docCheck.setContent(temp))
                lastMsg =QString::fromStdString(chatItem.nickInGroupChat) + ": " + readMsgFromXml(temp);
            else
            {
                lastMsg =QString::fromStdString(chatItem.nickInGroupChat) + ": " + lastMsgQstr;
            }
        }
        else
        {
             if (docCheck.setContent(temp))
                lastMsg = readMsgFromXml(temp);
            else lastMsg = lastMsgQstr;
        }

        //GET status of last msg
        QString lastMsgStatus =  (chatItem.isOtherLastMsg? "": "sent");


////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////// BEGIN TO CHOOSE value after PREPARING ALLs.                          ////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////

        try {
            switch (role) {
            case Role::Picture:
            case Qt::DecorationRole:

                return QPixmap::fromImage(getCirclePhoto(avatar, avatar.size().width() )) ;
            case Role::DisplayName:
            case Qt::DisplayRole:
            {
                return QVariant(QString::fromStdString(chatItem.displayName));
            }
            case Role::DisplayID:
            {
                return  QVariant(lastMsg);
            }
            case Role::Presence:
            {
                return QVariant(presenceForChat);
            }
            case Role::URI:
            {
                return QVariant(QString::fromStdString("unseenp2p.com"));
            }
            case Role::UnreadMessagesCount:
                return QVariant(chatItem.UnreadMessagesCount);
            case Role::LastInteractionDate:
            {
                return QVariant(timedateForMsgResult);
            }
            case Role::LastInteraction:
                return QVariant(lastMsgStatus);
            case Role::LastInteractionType:
                return QVariant(0);
            case Role::ContactType:
            {
                return QVariant(0);
            }
            case Role::UID:
                return QVariant(QString("1234"));
            case Role::ContextMenuOpen:
                return QVariant(isContextMenuOpen);
            }
        } catch (...) {}
    }

    return QVariant();
}

QModelIndex SmartListModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    if (column != 0) {
        return QModelIndex();
    }

    if (row >= 0 && row < rowCount()) {
        return createIndex(row, column);
    }
    return QModelIndex();
}

QModelIndex SmartListModel::parent(const QModelIndex &child) const
{
    Q_UNUSED(child);
    return QModelIndex();
}

Qt::ItemFlags SmartListModel::flags(const QModelIndex &index) const
{
    auto flags = QAbstractItemModel::flags(index) | Qt::ItemNeverHasChildren | Qt::ItemIsSelectable;
//    auto type = Utils::toEnum<lrc::api::profile::Type>(data(index, Role::ContactType).value<int>());
//    auto displayName = data(index, Role::DisplayName).value<QString>();
//    auto uid = data(index, Role::UID).value<QString>();
//    if (!index.isValid()) {
//        return QAbstractItemModel::flags(index);
//    } else if ( type == lrc::api::profile::Type::TEMPORARY &&
//                uid.isEmpty()) {
//        flags &= ~(Qt::ItemIsSelectable);
//    }
    return flags;
}

void
SmartListModel::setAccount(const std::string& accId)
{
    beginResetModel();
    accId_ = accId;
    endResetModel();
}
