#ifndef P3GXSCHATSERVICE_H
#define P3GXSCHATSERVICE_H

/*******************************************************************************
 * libretroshare/src/chat: chatservice.h                                       *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2004-2008 by Robert Fernie.                                       *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Lesser General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Lesser General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Lesser General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/



#include <list>
#include <string>
#include <vector>

#include "rsitems/rsmsgitems.h"
#include "services/p3service.h"
#include "pqi/pqiservicemonitor.h"
#include "chat/distantchat.h"
#include "chat/distributedchat.h"
#include "retroshare/rsmsgs.h"
#include "gxstrans/p3gxstrans.h"
#include "util/rsdeprecate.h"

#include "services/p3gxschats.h"
#include "rsitems/rsgxschatitems.h"

class p3ServiceControl;
class p3LinkMgr;
class p3HistoryMgr;

typedef RsPeerId ChatLobbyVirtualPeerId ;


//!The basic Chat service.
 /**
  *
  * Can be used to send and receive chats, immediate status (using notify), avatars, and custom status
  * This service uses rsnotify (callbacks librs clients (e.g. rs-gui))
  * @see NotifyBase
  */
class p3GxsChatService :
        public p3Service, public DistantChatService, public DistributedChatService,
        public pqiServiceMonitor, /*GxsTransClient, */ p3GxsChats
{
public:
    p3GxsChatService(p3ServiceControl *cs, p3IdService *pids, p3LinkMgr *cm,
                   p3HistoryMgr *historyMgr, /*p3GxsTrans& gxsTransService,*/ RsGeneralDataService *gds, RsNetworkExchangeService *nes,
                    RsGixs* gixs );



    virtual RsServiceInfo getServiceInfo();

    static const uint32_t FRAGMENT_SIZE;

    /***** overloaded from p3Service *****/
    /*!
         * This retrieves all chat msg items and also (important!)
         * processes chat-status items that are in service item queue. chat msg item requests are also processed and not returned
         * (important! also) notifications sent to notify base  on receipt avatar, immediate status and custom status
         * : notifyCustomState, notifyChatStatus, notifyPeerHasNewAvatar
         * @see NotifyBase
         */
    virtual int tick();

    /*************** pqiMonitor callback ***********************/
    virtual void statusChange(const std::list<pqiServicePeer> &plist);

    /*!
         * public chat sent to all peers
         */
    void sendPublicChat(const std::string &msg);

    /********* RsMsgs ***********/
    /*!
     * Send a chat message.
     * @param destination where to send the chat message
     * @param msg the message
     * @see ChatId
     */
    bool sendChat(ChatId destination, std::string msg);

    /*!
     * Send a chat gxs message.
     * @param token use to store sqllite message
     * @param groupId sending message with this groupid (groupId is conversation ID).
     * @param post sending gxs message with direct push messages.

     * @see publishMeg
     */
    bool sendGxsChat(uint32_t &token, RsGxsChatGroup &conversation, RsGxsChatMsg &post);


    /*!
         * chat is sent to specifc peer
         * @param id peer to send chat msg to
         */
    bool	sendPrivateChat(const RsPeerId &id, const std::string &msg);

    /**
     * can be used to send 'immediate' status msgs, these status updates are
     * meant for immediate use by peer (not saved by rs) e.g currently used to
     * update user when a peer 'is typing' during a chat */
    void sendStatusString( const ChatId& id, const std::string& status_str );

    /**
     * @brief clearChatLobby: Signal chat was cleared by GUI.
     * @param id: Chat id cleared.
     */
    virtual void clearChatLobby(const ChatId& id);

    /*!
         * send to all peers online
         *@see sendStatusString()
         */
    void  sendGroupChatStatusString(const std::string& status_str) ;

    /*!
         * this retrieves custom status for a peers, generate a requests to the peer
         * @param peer_id the id of the peer you want status string for
         */
    std::string getCustomStateString(const RsPeerId& peer_id) ;

    /*!
         * sets the client's custom status, generates 'status available' item sent to all online peers
         */
    void  setOwnCustomStateString(const std::string&) ;

    /*!
         * @return client's custom string
         */
    std::string getOwnCustomStateString() ;

    /*! gets the peer's avatar in jpeg format, if available. Null otherwise. Also asks the peer to send
        * its avatar, if not already available. Creates a new unsigned char array. It's the caller's
        * responsibility to delete this ones used.
        */
    void getAvatarJpegData(const RsPeerId& peer_id,unsigned char *& data,int& size) ;

    /*!
         * Sets the avatar data and size for client's account
         * @param data is copied, so should be destroyed by the caller
         */
    void setOwnAvatarJpegData(const unsigned char *data,int size) ;

    /*!
         * Gets the avatar data for clients account
         * data is in jpeg format
         */
    void getOwnAvatarJpegData(unsigned char *& data,int& size) ;

    /*!
         * Return the max message size for security forwarding
         * @param type RS_CHAT_TYPE_...
         * return 0 unlimited
         */
    static uint32_t getMaxMessageSecuritySize(int type);

    /*!
         * Checks message security, especially remove billion laughs attacks
         */

    static bool checkForMessageSecurity(RsChatMsgItem *) ;

    /*!
         * @param clear private chat queue
         */
    bool clearPrivateChatQueue(bool incoming, const RsPeerId &id);

    virtual bool initiateDistantChatConnexion( const RsGxsId& to_gxs_id,
                                               const RsGxsId& from_gxs_id,
                                               DistantChatPeerId &pid,
                                               uint32_t& error_code,
                                               bool notify = true );

    /// @see GxsTransClient::receiveGxsTransMail(...)
    virtual bool receiveGxsTransMail( const RsGxsId& authorId,
                                      const RsGxsId& recipientId,
                                      const uint8_t* data, uint32_t dataSize );

    /// @see GxsTransClient::notifySendMailStatus(...)
    virtual bool notifyGxsTransSendStatus( RsGxsTransId mailId,
                                           GxsTransSendStatus status );


protected:
    /************* from p3Config *******************/
    virtual RsSerialiser *setupSerialiser() ;

    /*!
         * chat msg items and custom status are saved
         */
    virtual bool saveList(bool& cleanup, std::list<RsItem*>&) ;
    virtual void saveDone();
    virtual bool loadList(std::list<RsItem*>& load) ;

    // accepts virtual peer id
    bool isOnline(const RsPeerId &pid) ;

    /// This is to be used by subclasses/parents to call IndicateConfigChanged()
    virtual void triggerConfigSave()  { IndicateConfigChanged() ; }

    /// Same, for storing messages in incoming list
    RS_DEPRECATED virtual void locked_storeIncomingMsg(RsChatMsgItem *) ;

private:
    RsMutex mChatMtx;

    class AvatarInfo ;
    class StateStringInfo ;

    // Receive chat queue
    void receiveChatQueue();
    void handleIncomingItem(RsItem *);	// called by the former, and turtle handler for incoming encrypted items

    virtual void sendChatItem(RsChatItem *) ;

    void initChatMessage(RsChatMsgItem *c, ChatMessage& msg);

    /// Send avatar info to peer in jpeg format.
    void sendAvatarJpegData(const RsPeerId& peer_id) ;

    /// Send custom state info to peer
    void sendCustomState(const RsPeerId& peer_id);

    /// Receive the avatar in a chat item, with RS_CHAT_RECEIVE_AVATAR flag.
    void receiveAvatarJpegData(RsChatAvatarItem *ci) ;	// new method
    void receiveStateString(const RsPeerId& id,const std::string& s) ;

    /// methods for handling various Chat items.
    virtual bool handleRecvChatMsgItem(RsChatMsgItem *&item) ;			// NULL-ifies the item if memory ownership is taken

    void handleRecvChatStatusItem(RsChatStatusItem *item) ;
    void handleRecvChatAvatarItem(RsChatAvatarItem *item) ;

    /// Sends a request for an avatar to the peer of given id
    void sendAvatarRequest(const RsPeerId& peer_id) ;

    /// Send a request for custom status string
    void sendCustomStateRequest(const RsPeerId& peer_id);

    /// called as a proxy to sendItem(). Possibly splits item into multiple items of size lower than the maximum item size.
    //void checkSizeAndSendMessage(RsChatLobbyMsgItem *item) ;
    void checkSizeAndSendMessage(RsChatMsgItem *item) ;	// keep for compatibility for a few weeks.

    /// Called when a RsChatMsgItem is received. The item may be collapsed with any waiting partial chat item from the same peer.
        /// if so, the chat item will be turned to NULL
    bool locked_checkAndRebuildPartialMessage(RsChatMsgItem *&) ;

    //gxs chat service
    bool sendGxsChatDirect(const RsPeerId &friendId, GxsNxsChatMsgItem *msgItem );
    bool sendGxsChatDistant(const RsGxsId & fromGxsid, const RsGxsId toGxsId, GxsNxsChatMsgItem *msgItem ){return true;}

    typedef std::vector<GxsNxsChatGroupItem*> GrpFragments;
    typedef std::vector<GxsNxsChatMsgItem*> MsgFragments;

    /*!
     * Loops over pending publish key orders.
     */
    void sharePublishKeysPending() ;

    /*!
     * Fragment a message into individual fragments which are at most 150kb
     * @param msg message to fragment
     * @param msgFragments fragmented message
     * @return false if fragmentation fails true otherwise
     */
    bool fragmentMsg(GxsNxsChatMsgItem& msg, MsgFragments& msgFragments) const;

    /*!
     * Fragment a group into individual fragments which are at most 150kb
     * @param grp group to fragment
     * @param grpFragments fragmented group
     * @return false if fragmentation fails true other wise
     */
    bool fragmentGrp(GxsNxsChatGroupItem& grp, GrpFragments& grpFragments) const;

    /*!
     * Fragment a message into individual fragments which are at most 150kb
     * @param msg message to fragment
     * @param msgFragments fragmented message
     * @return NULL if not possible to reconstruct message from fragment,
     *              pointer to defragments nxs message is possible
     */
    GxsNxsChatMsgItem* deFragmentMsg(MsgFragments& msgFragments) const;

    /*!
     * Fragment a group into individual fragments which are at most 150kb
     * @param grp group to fragment
     * @param grpFragments fragmented group
     * @return NULL if not possible to reconstruct group from fragment,
     *              pointer to defragments nxs group is possible
     */
    GxsNxsChatGroupItem* deFragmentGrp(GrpFragments& grpFragments) const;


    /*!
     * Note that if all fragments for a message are not found then its fragments are dropped
     * @param fragments message fragments which are not necessarily from the same message
     * @param partFragments the partitioned fragments (into message ids)
     */
    void collateMsgFragments(MsgFragments &fragments, std::map<RsGxsMessageId, MsgFragments>& partFragments) const;

    /*!
     * Note that if all fragments for a group are not found then its fragments are dropped
     * @param fragments group fragments which are not necessarily from the same group
     * @param partFragments the partitioned fragments (into message ids)
     */
    void collateGrpFragments(GrpFragments fragments, std::map<RsGxsGroupId, GrpFragments>& partFragments) const;

    /*!
    * stamp the group info from that particular peer at the given time.
    */

    RsChatAvatarItem *makeOwnAvatarItem() ;
    RsChatStatusItem *makeOwnCustomStateStringItem() ;

    p3ServiceControl *mServiceCtrl;
    p3LinkMgr *mLinkMgr;
    p3HistoryMgr *mHistoryMgr;

    /// messages waiting to be send when peer comes online
    typedef std::map<uint64_t, RsChatMsgItem *> outMP;
    outMP privateOutgoingMap;

    AvatarInfo *_own_avatar ;
    std::map<RsPeerId,AvatarInfo *> _avatars ;
    std::map<RsPeerId,RsChatMsgItem *> _pendingPartialMessages;

    std::string _custom_status_string ;
    std::map<RsPeerId,StateStringInfo> _state_strings ;

    RsChatSerialiser *_serializer;

    struct DistantEndpoints { RsGxsId from; RsGxsId to; };
    typedef std::map<DistantChatPeerId, DistantEndpoints> DIDEMap;
    DIDEMap mDistantGxsMap;
    RsMutex mDGMutex;

    //p3GxsTrans& mGxsTransport;
};

class p3GxsChatService::StateStringInfo
{
   public:
      StateStringInfo()
      {
          _custom_status_string = "" ;	// the custom status string of the peer
          _peer_is_new = false ;			// true when the peer has a new avatar
          _own_is_new = false ;				// true when I myself a new avatar to send to this peer.
      }

      std::string _custom_status_string ;
      int _peer_is_new ;			// true when the peer has a new avatar
      int _own_is_new ;			// true when I myself a new avatar to send to this peer.
};



#endif // P3GXSCHATSERVICE_H
