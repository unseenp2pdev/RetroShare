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
//#include <QMetaType>
//#include <QIODevice>
//#include <QByteArray>
//#include <QBuffer>
#include <QPainter>

#include "ChatLobbyWidget.h"
#include "MainWindow.h"
#include "retroshare/rspeers.h"
#include "retroshare/rsstatus.h"

#include "models/conversationmodel.h"

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


int SmartListModel::rowCount(const QModelIndex &parent) const
{
    //int count = MainWindow::getInstance()->getConversationModel()->countOfConversations();

    ChatLobbyWidget* chatWidget = MainWindow::getInstance()->chatLobbyDialog;
    int count = 0;
    if (chatWidget)
    {
        //try to get item in index.row() from gui
        //std::list<ChatItemStruct> _chatItemsList = chatWidget->getChatItemsList();
        ConversationModel* convModel = chatWidget->getConversationModel();
        std::vector<conversation::Info> list =  convModel->allFilteredConversations(); //->getConversationList();
        if (!list.empty())
        {
            count = static_cast<int> (list.size());
        }
    }
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

    ChatLobbyWidget* chatWidget = MainWindow::getInstance()->chatLobbyDialog;
    if (chatWidget)
    {
        //try to get item in index.row() from gui
        //std::list<ChatItemStruct> _chatItemsList = chatWidget->getChatItemsList();
        ConversationModel* convModel = chatWidget->getConversationModel();

        //conversation::Info chatItem = chatWidget->getConversationList().at(index.row());

        conversation::Info chatItem = convModel->allFilteredConversations().at(index.row());

        //Get avatar for groupchat or contact item
        RsPeerId peerId = chatItem.chatId.toPeerId();
        bool presenceForChat;
        if (chatItem.contactType == 0) presenceForChat = false;
        else
        {
            StatusInfo statusContactInfo;
            rsStatus->getStatus(peerId,statusContactInfo);
            switch (statusContactInfo.status)
            {
                case RS_STATUS_INACTIVE:
                    presenceForChat = false;
                    break;

                case RS_STATUS_ONLINE:
                case RS_STATUS_AWAY:
                case RS_STATUS_BUSY:
                    presenceForChat = true;
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
            avatar = chatWidget->avatarImageForPeerId(peerId);
        }

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
                return  QVariant(QString::fromStdString(chatItem.nickInGroupChat));
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
                return QVariant(4);
            case Role::LastInteractionDate:
            {
                //return QVariant("10:4");
                return QVariant(chatItem.LastInteractionDate);
            }
            case Role::LastInteraction:
                return QVariant(QString::fromStdString(chatItem.lastMessage));
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
