/****************************************************************************
 *    Copyright (C) 2017-2019 Savoir-faire Linux Inc.                                  *
 *   Author: Nicolas Jäger <nicolas.jager@savoirfairelinux.com>             *
 *   Author: Sébastien Blin <sebastien.blin@savoirfairelinux.com>           *
 *                                                                          *
 *   This library is free software; you can redistribute it and/or          *
 *   modify it under the terms of the GNU Lesser General Public             *
 *   License as published by the Free Software Foundation; either           *
 *   version 2.1 of the License, or (at your option) any later version.     *
 *                                                                          *
 *   This library is distributed in the hope that it will be useful,        *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU      *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU General Public License      *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.  *
 ***************************************************************************/
#pragma once

// Std
#include <vector>
#include <map>

#include <QObject>
#include <QDateTime>
#include <QIcon>

// Data
#include "interaction.h"
#include "retroshare/rsmsgs.h"

//namespace unseenp2p
//{

//namespace api
//{

namespace conversation
{

struct Info
{
//    std::string uid = "";
//    std::string accountId;
    //std::vector<std::string> participants;
    //std::string callId;
    //std::string confId;
    //std::map<uint64_t, interaction::Info> interactions; //for later use: other events: calling, receive file,...
    //uint64_t lastMessageUid = 0;
    //unsigned int unreadMessages = 0;

    //QIcon icon ;
    std::string displayName;            //group name or contact name
    std::string nickInGroupChat;        // only in groupchat
    unsigned int UnreadMessagesCount = 0;
    QDateTime LastInteractionDate;    // date for last message
    std::string lastMessage;
    int contactType;                // 0 - groupchat, 1 - contact chat
    int groupChatType;              // 0 - public, 1: private
    //ChatLobbyId groupId;            // for groupchat type
    ChatId chatId;                  // for contact (chatId.toPeerId) and groupchat (chatId.toLobbyId)
    QString uId;                    // unique Id: this will take groupId or chatId for Id for all list
    //Info(){}
    Info(std::string displayName, std::string nickInGroupChat,
         unsigned int UnreadMessagesCount, QDateTime LastInteractionDate, std::string lastMessage,
         int contactType, int groupChatType, ChatId chatId, QString uId): displayName(displayName),nickInGroupChat(nickInGroupChat),
           UnreadMessagesCount(UnreadMessagesCount), LastInteractionDate(LastInteractionDate),
            lastMessage(lastMessage), contactType(contactType), groupChatType(groupChatType), uId(uId) {}

//    Info(std::string displayName, std::string nickInGroupChat,
//         unsigned int UnreadMessagesCount, std::string lastMessage,
//         int contactType, int groupChatType): displayName(displayName),nickInGroupChat(nickInGroupChat),
//           UnreadMessagesCount(UnreadMessagesCount),
//            lastMessage(lastMessage), contactType(contactType), groupChatType(groupChatType) {}

};

} // namespace conversation
//} // namespace api
//} // namespace unseenp2p
