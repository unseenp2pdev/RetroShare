/****************************************************************************
 *    Copyright (C) 2017-2019 Savoir-faire Linux Inc.                             *
 *   Author: Nicolas Jäger <nicolas.jager@savoirfairelinux.com>             *
 *   Author: Sébastien Blin <sebastien.blin@savoirfairelinux.com>           *
 *   Author: Guillaume Roguez <guillaume.roguez@savoirfairelinux.com>       *
 *   Author: Kateryna Kostiuk <kateryna.kostiuk@savoirfairelinux.com>       *
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
#include "conversationmodel.h"

//Qt
#include <QtCore/QTimer>
#include <QFileInfo>

// std
#include <algorithm>
#include <mutex>
#include <regex>
#include <fstream>
#include <vector>
//
#include "behaviorcontroller.h"


//namespace unseenp2p
//{

//using namespace authority;
//namespace api
//{
    //namespace interaction { struct Info; }

//class ConversationModelPimpl : public QObject
//{
//    Q_OBJECT
//public:
//    ConversationModelPimpl(const ConversationModel& linked);
//    ~ConversationModelPimpl();

//    std::vector<conversation::Info> conversationList();

//    const ConversationModel& _linked;

//    std::vector<conversation::Info> Conversations;

//};



ConversationModel::ConversationModel()
{
    //pimpl_ = new ConversationModelPimpl(*this);
    Conversations.reserve(200);
}

ConversationModel::~ConversationModel()
{

}



//conversation::Info
//ConversationModel::filteredConversation(const unsigned int row) const
//{
//    const auto& conversations = allFilteredConversations();
////    if (row >= conversations.size())
////        return NULL;

//    auto conversationInfo = conversations.at(row);

//    return conversationInfo;
//}

void
ConversationModel::makePermanent(const std::string& uid)
{
}

void
ConversationModel::selectConversation(const std::string& uid) const
{
    // Get conversation
//    auto conversationIdx = pimpl_->indexOf(uid);

//    if (conversationIdx == -1)
//        return;

//    if (uid.empty()) {
//        // if we select the temporary contact, check if its a valid contact.
//        return;
//    }


//    auto& conversation = conversations.at(conversationIdx);
//    // emit pimpl_->behaviorController.showChatView(conversation);


}

void
ConversationModel::removeConversation(const std::string& uid, bool banned)
{
    // Get conversation
//    auto conversationIdx = pimpl_->indexOf(uid);
//    if (conversationIdx == -1)
//        return;

//    auto& conversation = pimpl_->conversations.at(conversationIdx);

}

void
ConversationModel::deleteObsoleteHistory(int days)
{

}

void
ConversationModel::placeAudioOnlyCall(const std::string& uid)
{
//    pimpl_->placeCall(uid, true);
}

void
ConversationModel::placeCall(const std::string& uid)
{
//    pimpl_->placeCall(uid);
}

void
ConversationModel::sendMessage(const std::string& uid, const std::string& body)
{
}

void
ConversationModel::refreshFilter()
{

}

void
ConversationModel::setFilter(const std::string& filter)
{
}

//void
//ConversationModel::setFilter(const profile::Type& filter)
//{
//    // Switch between PENDING, RING and SIP contacts.
////    pimpl_->typeFilter = filter;
////    pimpl_->dirtyConversations = {true, true};
////    emit filterChanged();
//}

void
ConversationModel::joinConversations(const std::string& uidA, const std::string& uidB)
{
}

void
ConversationModel::clearHistory(const std::string& uid)
{
}

void
ConversationModel::clearInteractionFromConversation(const std::string& convId, const uint64_t& interactionId)
{
}

void
ConversationModel::retryInteraction(const std::string& convId, const uint64_t& interactionId)
{

}

void
ConversationModel::clearAllHistory()
{
}

void
ConversationModel::setInteractionRead(const std::string& convId,
                                      const uint64_t& interactionId)
{
}

void
ConversationModel::clearUnreadInteractions(const std::string& convId) {

}

void
ConversationModel::sendFile(const std::string& convUid,
                            const std::string& path,
                            const std::string& filename)
{
}

void
ConversationModel::acceptTransfer(const std::string& convUid, uint64_t interactionId, const std::string& path)
{
//    pimpl_->acceptTransfer(convUid, interactionId, path);
}

void
ConversationModel::cancelTransfer(const std::string& convUid, uint64_t interactionId)
{
    // For this action, we change interaction status before effective canceling as daemon will
    // emit Finished event code immediately (before leaving this method) in non-DBus mode.

}

//void
//ConversationModel::getTransferInfo(uint64_t interactionId, datatransfer::Info& info)
//{

//}

int
ConversationModel::getNumberOfUnreadMessagesFor(const std::string& convUid)
{
//    return pimpl_->getNumberOfUnreadMessagesFor(convUid);
}

//ConversationModelPimpl::ConversationModelPimpl(const ConversationModel& linked):_linked(linked)
//{
//}
//ConversationModelPimpl::~ConversationModelPimpl()
//{
//}

//std::vector<conversation::Info> ConversationModelPimpl::conversationList()
//{
//    return Conversations;
//}

//}   //namespace api
//} // namespace unseenp2p

//#include "models/moc_conversationmodel.cpp"
//#include "conversationmodel.moc"
