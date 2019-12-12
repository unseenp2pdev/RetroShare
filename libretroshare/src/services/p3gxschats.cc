/*******************************************************************************
 * libretroshare/src/services: p3GxsChats.cc                                *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2012-2012 Robert Fernie <retroshare@lunamutt.com>                 *
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
#include "services/p3gxschats.h"
#include "rsitems/rsgxschatitems.h"
#include "util/radix64.h"
#include "util/rsmemory.h"

#include <retroshare/rsidentity.h>
#include <retroshare/rsfiles.h>


#include "retroshare/rsgxsflags.h"
#include "retroshare/rsfiles.h"

#include "rsserver/p3face.h"
#include "retroshare/rsnotify.h"

#include <stdio.h>

// For Dummy Msgs.
#include "util/rsrandom.h"
#include "util/rsstring.h"

#include "chat/rschatitems.h"
#include "retroshare/rspeers.h"
#include "rsitems/rsnxsitems.h"

#define GXSCHATS_DEBUG 1



RsGxsChats *rsGxsChats = NULL;


#define GXSCHAT_STOREPERIOD	(3600 * 24 * 30)  // 30 days store

#define	 GXSCHATS_SUBSCRIBED_META		11
#define  GXSCHATS_UNPROCESSED_SPECIFIC	12
#define  GXSCHATS_UNPROCESSED_GENERIC	13

#define CHAT_PROCESS	 		0x0011
#define CHAT_TESTEVENT_DUMMYDATA	0x0012
#define DUMMYDATA_PERIOD		60	// long enough for some RsIdentities to be generated.

#define CHAT_DOWNLOAD_PERIOD 	(3600 * 24 * 7)
#define CHAT_MAX_AUTO_DL		(8 * 1024 * 1024 * 1024ull)	// 8 GB. Just a security ;-)

/********************************************************************************/
/******************* Startup / Tick    ******************************************/
/********************************************************************************/

p3GxsChats::p3GxsChats(RsGeneralDataService *gds, RsNetworkExchangeService *nes, RsGixs* gixs):
    RsGenExchange( gds, nes, new RsGxsChatSerialiser(), RS_SERVICE_GXS_TYPE_CHATS, gixs, chatsAuthenPolicy() ),
    RsGxsChats(static_cast<RsGxsIface&>(*this)), GxsTokenQueue(this),
    mSearchCallbacksMapMutex("GXS chats search"),
    mChatSrv(NULL),
    mSerialiser(new RsGxsChatSerialiser()),
    mChatMtx("GxsChat Mutex")
{
    // For Dummy Msgs.
    mGenActive = false;
    mCommentService = new p3GxsCommentService(this,  RS_SERVICE_GXS_TYPE_CHATS);

    RsTickEvent::schedule_in(CHAT_PROCESS, 0);

    // Test Data disabled in repo.
    //RsTickEvent::schedule_in(CHAT_TESTEVENT_DUMMYDATA, DUMMYDATA_PERIOD);
    mGenToken = 0;
    mGenCount = 0;

}

const std::string GXS_CHATS_APP_NAME = "gxschats";
const uint16_t GXS_CHATS_APP_MAJOR_VERSION  =       1;
const uint16_t GXS_CHATS_APP_MINOR_VERSION  =       0;
const uint16_t GXS_CHATS_MIN_MAJOR_VERSION  =       1;
const uint16_t GXS_CHATS_MIN_MINOR_VERSION  =       0;

RsServiceInfo p3GxsChats::getServiceInfo()
{
        return RsServiceInfo(RS_SERVICE_GXS_TYPE_CHATS,
                GXS_CHATS_APP_NAME,
                GXS_CHATS_APP_MAJOR_VERSION,
                GXS_CHATS_APP_MINOR_VERSION,
                GXS_CHATS_MIN_MAJOR_VERSION,
                GXS_CHATS_MIN_MINOR_VERSION);
}

uint32_t p3GxsChats::chatsAuthenPolicy()
{
    uint32_t policy = 0;
    uint32_t flag = 0;

    flag = GXS_SERV::MSG_AUTHEN_ROOT_PUBLISH_SIGN | GXS_SERV::MSG_AUTHEN_CHILD_AUTHOR_SIGN;
    RsGenExchange::setAuthenPolicyFlag(flag, policy, RsGenExchange::PUBLIC_GRP_BITS);

    flag |= GXS_SERV::MSG_AUTHEN_CHILD_PUBLISH_SIGN;
    RsGenExchange::setAuthenPolicyFlag(flag, policy, RsGenExchange::RESTRICTED_GRP_BITS);
    RsGenExchange::setAuthenPolicyFlag(flag, policy, RsGenExchange::PRIVATE_GRP_BITS);

    flag = 0;
    RsGenExchange::setAuthenPolicyFlag(flag, policy, RsGenExchange::GRP_OPTION_BITS);

    return policy;
}

static const uint32_t GXS_CHATS_CONFIG_MAX_TIME_NOTIFY_STORAGE = 86400*30*2 ; // ignore notifications for 2 months
static const uint8_t  GXS_CHATS_CONFIG_SUBTYPE_NOTIFY_RECORD   = 0x02 ;

struct RsGxsChatNotifyRecordsItem: public RsItem
{

    RsGxsChatNotifyRecordsItem()
        : RsItem(RS_PKT_VERSION_SERVICE,RS_SERVICE_GXS_TYPE_CHATS_CONFIG,GXS_CHATS_CONFIG_SUBTYPE_NOTIFY_RECORD)
    {}

    virtual ~RsGxsChatNotifyRecordsItem() {}

    void serial_process( RsGenericSerializer::SerializeJob j,
                         RsGenericSerializer::SerializeContext& ctx )
    { RS_SERIAL_PROCESS(records); }

    void clear() {}

    std::map<RsGxsGroupId,rstime_t> records;
};


class GxsChatsConfigSerializer : public RsServiceSerializer
{
public:
    GxsChatsConfigSerializer() : RsServiceSerializer(RS_SERVICE_GXS_TYPE_CHATS_CONFIG) {}
    virtual ~GxsChatsConfigSerializer() {}

    RsItem* create_item(uint16_t service_id, uint8_t item_sub_id) const
    {
        if(service_id != RS_SERVICE_GXS_TYPE_CHATS_CONFIG)
            return NULL;

        switch(item_sub_id)
        {
        case GXS_CHATS_CONFIG_SUBTYPE_NOTIFY_RECORD: return new RsGxsChatNotifyRecordsItem();
        default:
            return NULL;
        }
    }
};

bool p3GxsChats::saveList(bool &cleanup, std::list<RsItem *>&saveList)
{
    cleanup = true ;

    RsGxsChatNotifyRecordsItem *item = new RsGxsChatNotifyRecordsItem ;

    item->records = mKnownChats ;

    saveList.push_back(item) ;
    return true;
}

bool p3GxsChats::loadList(std::list<RsItem *>& loadList)
{
    while(!loadList.empty())
    {
        RsItem *item = loadList.front();
        loadList.pop_front();

        rstime_t now = time(NULL);

        RsGxsChatNotifyRecordsItem *fnr = dynamic_cast<RsGxsChatNotifyRecordsItem*>(item) ;

        if(fnr != NULL)
        {
            mKnownChats.clear();

            for(auto it(fnr->records.begin());it!=fnr->records.end();++it)
                if( it->second + GXS_CHATS_CONFIG_MAX_TIME_NOTIFY_STORAGE < now)
                    mKnownChats.insert(*it) ;
        }

        delete item ;
    }
    return true;
}

RsSerialiser* p3GxsChats::setupSerialiser()
{
    RsSerialiser* rss = new RsSerialiser;
    rss->addSerialType(new GxsChatsConfigSerializer());

    return rss;
}

/** Overloaded to cache new groups **/
RsGenExchange::ServiceCreate_Return p3GxsChats::service_CreateGroup(RsGxsGrpItem* grpItem, RsTlvSecurityKeySet& /* keySet */)
{
    updateSubscribedGroup(grpItem->meta);

    bool serialOk = false;
    RsNxsGrp* grp = new RsNxsGrp(RS_PKT_SUBTYPE_NXS_CHAT_GRP_ITEM);

    uint32_t size = mSerialiser->size(grpItem);
    char *gData = new char[size];
    serialOk = mSerialiser->serialise(grpItem, gData, &size);
    if (serialOk){  //push group to your peers before GxsSync Start!
        grp->grp.setBinData(gData, size);

        grp->metaData = new RsGxsGrpMetaData();
        *(grp->metaData) = grpItem->meta;

        // TODO: change when publish key optimisation added (public groups don't have publish key
        grp->metaData->mSubscribeFlags = GXS_SERV::GROUP_SUBSCRIBE_ADMIN | GXS_SERV::GROUP_SUBSCRIBE_SUBSCRIBED
                        | GXS_SERV::GROUP_SUBSCRIBE_PUBLISH;

        //send Group to the network

        //mChatSrv->sendGxsItem(grp);  //need to include a list of members or getting circle or private node members.
        delete grp;
        delete[] gData;

    } //end pushing group to all add members.
    return SERVICE_CREATE_SUCCESS;
}

RsGenExchange::ServiceCreate_Return p3GxsChats::service_CreateMessage(RsNxsMsg* msg){
#ifdef GXSCHATS_DEBUG
    std::cerr << "p3GxsChats::service_CreateMessage()  MsgId: " << msg->msgId << " and GroupId: " << msg->grpId<< std::endl;
#endif

        GxsNxsChatMsgItem *mItem = new GxsNxsChatMsgItem(RS_PKT_SUBTYPE_GXSCHAT_MSG);

        if (mSerialiser->size(msg)){
            mItem->grpId     = msg->grpId;
            mItem->msg.setBinData(msg->msg.bin_data,msg->msg.bin_len);
            mItem->meta.setBinData(msg->meta.bin_data,msg->meta.bin_len);

            mItem->metaData  = msg->metaData;
            mItem->msgId     = msg->msgId;


            std::cerr << "***********p3GxsChats::service_CreateMessage: Info*************************"<<std::endl;
            std::cerr << "MessageId:"<<msg->msgId << " and groupId: "<<msg->grpId << " and Message Size: "<<msg->msg.TlvSize()<<std::endl;
            std::cerr << "Mesage= "; msg->msg.print(std::cerr, 15); std::cerr<<std::endl;
            std::cerr <<"   Meta Size:"<<msg->meta.TlvSize()<<std::endl;
            std::cerr <<"   mAuthorId:" <<msg->metaData->mAuthorId << std::endl;
            std::cerr <<"   mChildTs:"  <<msg->metaData->mChildTs << std::endl;
            std::cerr <<"   mGroupId:" <<msg->metaData->mGroupId << std::endl;
            std::cerr <<"   mHash:" <<msg->metaData->mHash << std::endl;
            std::cerr <<"   mMsgFlags:" <<msg->metaData->mMsgFlags << std::endl;
            std::cerr <<"   mMsgId:" <<msg->metaData->mMsgId << std::endl;
            std::cerr <<"   mMsgName:" <<msg->metaData->mMsgName << std::endl;
            std::cerr <<"   mMsgSize:" <<msg->metaData->mMsgSize << std::endl;
            std::cerr <<"   mMsgStatus:" <<msg->metaData->mMsgStatus << std::endl;
            std::cerr <<"   mOrigMsgId:" <<msg->metaData->mOrigMsgId << std::endl;
            std::cerr <<"   mParentId:" <<msg->metaData->mParentId << std::endl;
            std::cerr <<"   mPublishTs:" <<msg->metaData->mPublishTs << std::endl;
            std::cerr <<"   mServiceString:" <<msg->metaData->mServiceString << std::endl;
            std::cerr <<"   mThreadId:" <<msg->metaData->mThreadId << std::endl;
            std::cerr <<"   recvTS:" <<msg->metaData->recvTS << std::endl;
            std::cerr <<"   refcount:" <<msg->metaData->refcount << std::endl;
            std::cerr <<"   signSet Size:" <<msg->metaData->signSet.TlvSize() << std::endl;
            msg->metaData->signSet.print(std::cerr, 15); std::cerr<<std::endl;

            //messageId created after the createMessage function call.
            //sendChatMessage(mItem);
         }

        std::list<RsPeerId> ids;
        rsPeers->getOnlineList(ids);

        RsNetworkExchangeService *netService = RsGenExchange::getNetworkExchangeService();

        for (auto it = ids.begin(); it != ids.end(); it++){
            RsNxsMsg *newMsg = new RsNxsMsg(msg->PacketService());
            newMsg->PeerId(*it);
            newMsg->grpId = msg->grpId;
            newMsg->msgId = msg->msgId;
            newMsg->msg.setBinData(msg->msg.bin_data, msg->msg.bin_len);
            newMsg->meta.setBinData(msg->meta.bin_data, msg->meta.bin_len);
            netService->PublishChat(newMsg);
        }

    return SERVICE_CREATE_SUCCESS;
}

void p3GxsChats::sendChatMessage(GxsNxsChatMsgItem *&msg ){

    std::map<RsGxsGroupId, RsGroupMetaData>::iterator it;
    it = mSubscribedGroups.find(msg->grpId);

    //mSubscribedGroups.erase(it);
    RsGroupMetaData chatGrpMeta =it->second;

    switch(chatGrpMeta.mCircleType){
        case GXS_CIRCLE_TYPE_PUBLIC:
        {
            //get all online friends and send this message to them.
            mChatSrv->sendGxsPubChat(msg);
            std::cerr <<"Sending Gxs MessageID: "<<msg->msgId <<std::endl;
            std::cerr <<"Sending GroupdId: "<<msg->grpId<<std::endl;
            break;
        }
        case GXS_CIRCLE_TYPE_EXTERNAL:// restricted to an external circle, made of RsGxsId
        {
            std::cerr << "CircleType: GXS_CIRCLE_TYPE_EXTERNAL"<<std::endl;
            break;
        }
        case GXS_CIRCLE_TYPE_YOUR_FRIENDS_ONLY:	// restricted to a subset of friend nodes of a given RS node given by a RsPgpId list
        {
            if(!chatGrpMeta.mInternalCircle.isNull())
            {
                RsGroupInfo ginfo ;
                RsNodeGroupId  groupId = RsNodeGroupId(chatGrpMeta.mInternalCircle);

                if(rsPeers->getGroupInfo(groupId,ginfo))
                {
                    for (auto it = ginfo.peerIds.begin(); it != ginfo.peerIds.end(); it++ ){
                        std::list<RsPeerId> ids;
                        if (rsPeers->getAssociatedSSLIds(*it,ids)) {
                            mChatSrv->sendGxsChat(msg,ids);
                            ids.clear();
                            std::cerr <<"Sent Gxs MessageID: "<<msg->msgId <<std::endl;
                            std::cerr <<"Sent GroupdId: "<<msg->grpId<<std::endl;
                        }
                    }//end for loop
                }
            } //end case
            break;
        }

    }//end switch
}

void p3GxsChats::notifyChanges(std::vector<RsGxsNotify *> &changes)
{
#ifdef GXSCHATS_DEBUG
    std::cerr << "p3GxsChats::notifyChanges() : " << changes.size() << " changes to notify" << std::endl;
#endif

    p3Notify *notify = NULL;
    if (!changes.empty())
    {
        notify = RsServer::notify();
    }

    /* iterate through and grab any new messages */
    std::list<RsGxsGroupId> unprocessedGroups;

    std::vector<RsGxsNotify *>::iterator it;
    for(it = changes.begin(); it != changes.end(); ++it)
    {
        RsGxsMsgChange *msgChange = dynamic_cast<RsGxsMsgChange *>(*it);
        if (msgChange)
        {
            if (msgChange->getType() == RsGxsNotify::TYPE_RECEIVED_NEW)
            {
                /* message received */
                if (notify)
                {
                    std::map<RsGxsGroupId, std::set<RsGxsMessageId> > &msgChangeMap = msgChange->msgChangeMap;
                    for (auto mit = msgChangeMap.begin(); mit != msgChangeMap.end(); ++mit)
                        for (auto mit1 = mit->second.begin(); mit1 != mit->second.end(); ++mit1)
                        {
                            notify->AddFeedItem(RS_FEED_ITEM_CHATS_MSG, mit->first.toStdString(), mit1->toStdString());
                        }
                }
            }

            if (!msgChange->metaChange())
            {
#ifdef GXSCHATS_DEBUG
                std::cerr << "p3GxsChats::notifyChanges() Found Message Change Notification";
                std::cerr << std::endl;
#endif

                std::map<RsGxsGroupId, std::set<RsGxsMessageId> > &msgChangeMap = msgChange->msgChangeMap;
                for(auto mit = msgChangeMap.begin(); mit != msgChangeMap.end(); ++mit)
                {
#ifdef GXSCHATS_DEBUG
                    std::cerr << "p3GxsChats::notifyChanges() Msgs for Group: " << mit->first;
                    std::cerr << std::endl;
#endif
                                bool enabled = false ;

                }
            }
        }
        else
        {
            if (notify)
            {
                RsGxsGroupChange *grpChange = dynamic_cast<RsGxsGroupChange*>(*it);
                if (grpChange)
                {
                    switch (grpChange->getType())
                    {
                    default:
                        case RsGxsNotify::TYPE_PROCESSED:
                        case RsGxsNotify::TYPE_PUBLISHED:
                            break;

                        case RsGxsNotify::TYPE_RECEIVED_NEW:
                        {
                            /* group received */
                            std::list<RsGxsGroupId> &grpList = grpChange->mGrpIdList;
                            std::list<RsGxsGroupId>::iterator git;
                            for (git = grpList.begin(); git != grpList.end(); ++git)
                            {
                                if(mKnownChats.find(*git) == mKnownChats.end())
                                {
                                    notify->AddFeedItem(RS_FEED_ITEM_CHATS_NEW, git->toStdString());
                                    mKnownChats.insert(std::make_pair(*git,time(NULL))) ;
                                }
                                else
                                    std::cerr << "(II) Not notifying already known chat " << *git << std::endl;
                            }
                            break;
                        }

                        case RsGxsNotify::TYPE_RECEIVED_PUBLISHKEY:
                        {
                            /* group received */
                            std::list<RsGxsGroupId> &grpList = grpChange->mGrpIdList;
                            std::list<RsGxsGroupId>::iterator git;
                            for (git = grpList.begin(); git != grpList.end(); ++git)
                            {
                                notify->AddFeedItem(RS_FEED_ITEM_CHATS_PUBLISHKEY, git->toStdString());
                            }
                            break;
                        }
                    }
                }
            }
        }

        /* shouldn't need to worry about groups - as they need to be subscribed to */
    }

    request_SpecificSubscribedGroups(unprocessedGroups);

    RsGxsIfaceHelper::receiveChanges(changes);
}

void	p3GxsChats::service_tick()
{

static  rstime_t last_dummy_tick = 0;
//static  rstime_t last_chat_tick =0;

    if (time(NULL) > last_dummy_tick + 5)
    {
        dummy_tick();
        last_dummy_tick = time(NULL);

    }

//    if (time(NULL) > last_chat_tick + 5)
//    {
//        p3GxsChatService::tick();
//        last_chat_tick = time(NULL);

//    }
//    p3ChatService::tick();
    RsTickEvent::tick_events();
    GxsTokenQueue::checkRequests();

}

bool p3GxsChats::getGroupData(const uint32_t &token, std::vector<RsGxsChatGroup> &groups)
{
#ifdef GXSCHATS_DEBUG
    std::cerr << "p3GxsChats::getGroupData()";
    std::cerr << std::endl;
#endif

    std::vector<RsGxsGrpItem*> grpData;
    bool ok = RsGenExchange::getGroupData(token, grpData);

    if(ok)
    {
        std::vector<RsGxsGrpItem*>::iterator vit = grpData.begin();

        for(; vit != grpData.end(); ++vit)
        {
            RsGxsChatGroupItem* item = dynamic_cast<RsGxsChatGroupItem*>(*vit);
            if (item)
            {
                RsGxsChatGroup grp;
                item->toChatGroup(grp, true);
                delete item;
                groups.push_back(grp);
            }
            else
            {
                std::cerr << "p3GxsChats::getGroupData() ERROR in decode";
                std::cerr << std::endl;
                delete(*vit);
            }
        }
    }
    else
    {
        std::cerr << "p3GxsChats::getGroupData() ERROR in request";
        std::cerr << std::endl;
    }

    return ok;
}

bool p3GxsChats::groupShareKeys(
        const RsGxsGroupId &groupId, const std::set<RsPeerId>& peers )
{
    RsGenExchange::shareGroupPublishKey(groupId,peers);
    return true;
}

bool p3GxsChats::getPostData(const uint32_t &token, std::vector<RsGxsChatMsg> &msgs, std::vector<RsGxsComment> &cmts)
{
#ifdef GXSCHATS_DEBUG
    std::cerr << "p3GxsChats::getPostData()";
    std::cerr << std::endl;
#endif

    GxsMsgDataMap msgData;
    bool ok = RsGenExchange::getMsgData(token, msgData);

    if(ok)
    {
        GxsMsgDataMap::iterator mit = msgData.begin();

        for(; mit != msgData.end();  ++mit)
        {
            std::vector<RsGxsMsgItem*>& msgItems = mit->second;
            std::vector<RsGxsMsgItem*>::iterator vit = msgItems.begin();

            for(; vit != msgItems.end(); ++vit)
            {
                RsGxsChatMsgItem* postItem = dynamic_cast<RsGxsChatMsgItem*>(*vit);

                if(postItem)
                {
                    RsGxsChatMsg msg;
                    postItem->toChatPost(msg, true);
                    msgs.push_back(msg);
                    delete postItem;
                }
                else
                {
                    RsGxsCommentItem* cmtItem = dynamic_cast<RsGxsCommentItem*>(*vit);
                    if(cmtItem)
                    {
                        RsGxsComment cmt;
                        RsGxsMsgItem *mi = (*vit);
                        cmt = cmtItem->mMsg;
                        cmt.mMeta = mi->meta;
#ifdef GXSCOMMENT_DEBUG
                        std::cerr << "p3GxsChats::getPostData Found Comment:" << std::endl;
                        cmt.print(std::cerr,"  ", "cmt");
#endif
                        cmts.push_back(cmt);
                        delete cmtItem;
                    }
                    else
                    {
                        RsGxsMsgItem* msg = (*vit);
                        //const uint16_t RS_SERVICE_GXS_TYPE_CHATS    = 0x0231;
                        //const uint8_t  RS_PKT_SUBTYPE_GXSCHATS_POST_ITEM = 0x03;
                        //const uint8_t  RS_PKT_SUBTYPE_GXSCOMMENT_COMMENT_ITEM = 0xf1;
                        std::cerr << "Not a GxschatPostItem neither a RsGxsCommentItem"
                                            << " PacketService=" << std::hex << (int)msg->PacketService() << std::dec
                                            << " PacketSubType=" << std::hex << (int)msg->PacketSubType() << std::dec
                                            << " , deleting!" << std::endl;
                        delete *vit;
                    }
                }
            }
        }
    }
    else
    {
        std::cerr << "p3GxsChats::getPostData() ERROR in request";
        std::cerr << std::endl;
    }

    return ok;
}


void p3GxsChats::request_AllSubscribedGroups()
{
#ifdef GXSCHATS_DEBUG
    std::cerr << "p3GxsChats::request_SubscribedGroups()";
    std::cerr << std::endl;
#endif // GXSCHATS_DEBUG

    uint32_t ansType = RS_TOKREQ_ANSTYPE_SUMMARY;
    RsTokReqOptions opts;
    opts.mReqType = GXS_REQUEST_TYPE_GROUP_META;

    uint32_t token = 0;

    RsGenExchange::getTokenService()->requestGroupInfo(token, ansType, opts);
    GxsTokenQueue::queueRequest(token, GXSCHATS_SUBSCRIBED_META);

#define PERIODIC_ALL_PROCESS	300 // TESTING every 5 minutes.
    RsTickEvent::schedule_in(CHAT_PROCESS, PERIODIC_ALL_PROCESS);
}


void p3GxsChats::request_SpecificSubscribedGroups(const std::list<RsGxsGroupId> &groups)
{
#ifdef GXSCHATS_DEBUG
    std::cerr << "p3GxsChats::request_SpecificSubscribedGroups()";
    std::cerr << std::endl;
#endif // GXSCHATS_DEBUG

    uint32_t ansType = RS_TOKREQ_ANSTYPE_SUMMARY;
    RsTokReqOptions opts;
    opts.mReqType = GXS_REQUEST_TYPE_GROUP_META;

    uint32_t token = 0;

    RsGenExchange::getTokenService()->requestGroupInfo(token, ansType, opts, groups);
    GxsTokenQueue::queueRequest(token, GXSCHATS_SUBSCRIBED_META);
}


void p3GxsChats::load_SubscribedGroups(const uint32_t &token)
{
#ifdef GXSCHATS_DEBUG
    std::cerr << "p3GxsChats::load_SubscribedGroups()";
    std::cerr << std::endl;
#endif // GXSCHATS_DEBUG

    std::list<RsGroupMetaData> groups;
    std::list<RsGxsGroupId> groupList;

    getGroupMeta(token, groups);

    std::list<RsGroupMetaData>::iterator it;
    for(it = groups.begin(); it != groups.end(); ++it)
    {
        if (it->mSubscribeFlags &
            (GXS_SERV::GROUP_SUBSCRIBE_ADMIN |
            GXS_SERV::GROUP_SUBSCRIBE_PUBLISH |
            GXS_SERV::GROUP_SUBSCRIBE_SUBSCRIBED ))
        {
#ifdef GXSCHATS_DEBUG
            std::cerr << "p3GxsChats::load_SubscribedGroups() updating Subscribed Group: " << it->mGroupId;
            std::cerr << std::endl;
#endif

            updateSubscribedGroup(*it);
            bool enabled = false ;

        }
        else
        {
#ifdef GXSCHATS_DEBUG
            std::cerr << "p3GxsChats::load_SubscribedGroups() clearing unsubscribed Group: " << it->mGroupId;
            std::cerr << std::endl;
#endif
            clearUnsubscribedGroup(it->mGroupId);
        }
    }

    /* Query for UNPROCESSED POSTS from checkGroupList */
    request_GroupUnprocessedPosts(groupList);
}



void p3GxsChats::updateSubscribedGroup(const RsGroupMetaData &group)
{
#ifdef GXSCHATS_DEBUG
    std::cerr << "p3GxsChats::updateSubscribedGroup() id: " << group.mGroupId;
    std::cerr << std::endl;
#endif

    mSubscribedGroups[group.mGroupId] = group;
}


void p3GxsChats::clearUnsubscribedGroup(const RsGxsGroupId &id)
{
#ifdef GXSCHATS_DEBUG
    std::cerr << "p3GxsChats::clearUnsubscribedGroup() id: " << id;
    std::cerr << std::endl;
#endif

    //std::map<RsGxsGroupId, RsGrpMetaData> mSubscribedGroups;
    std::map<RsGxsGroupId, RsGroupMetaData>::iterator it;

    it = mSubscribedGroups.find(id);
    if (it != mSubscribedGroups.end())
    {
        mSubscribedGroups.erase(it);
    }
}


bool p3GxsChats::subscribeToGroup(uint32_t &token, const RsGxsGroupId &groupId, bool subscribe)
{
#ifdef GXSCHATS_DEBUG
    std::cerr << "p3GxsChats::subscribedToGroup() id: " << groupId << " subscribe: " << subscribe;
    std::cerr << std::endl;
#endif

    std::list<RsGxsGroupId> groups;
    groups.push_back(groupId);

    // Call down to do the real work.
    bool response = RsGenExchange::subscribeToGroup(token, groupId, subscribe);

    // reload Group afterwards.
    request_SpecificSubscribedGroups(groups);

    return response;
}


void p3GxsChats::request_SpecificUnprocessedPosts(std::list<std::pair<RsGxsGroupId, RsGxsMessageId> > &ids)
{
#ifdef GXSCHATS_DEBUG
    std::cerr << "p3GxsChats::request_SpecificUnprocessedPosts()";
    std::cerr << std::endl;
#endif // GXSCHATS_DEBUG

    uint32_t ansType = RS_TOKREQ_ANSTYPE_DATA;
    RsTokReqOptions opts;
    opts.mReqType = GXS_REQUEST_TYPE_MSG_DATA;

    // Only Fetch UNPROCESSED messages.
    opts.mStatusFilter = GXS_SERV::GXS_MSG_STATUS_UNPROCESSED;
    opts.mStatusMask = GXS_SERV::GXS_MSG_STATUS_UNPROCESSED;

    uint32_t token = 0;

    /* organise Ids how they want them */
    GxsMsgReq msgIds;
    std::list<std::pair<RsGxsGroupId, RsGxsMessageId> >::iterator it;
    for(it = ids.begin(); it != ids.end(); ++it)
        msgIds[it->first].insert(it->second);

    RsGenExchange::getTokenService()->requestMsgInfo(token, ansType, opts, msgIds);
    GxsTokenQueue::queueRequest(token, GXSCHATS_UNPROCESSED_SPECIFIC);
}


void p3GxsChats::request_GroupUnprocessedPosts(const std::list<RsGxsGroupId> &grouplist)
{
#ifdef GXSCHATS_DEBUG
    std::cerr << "p3GxsChats::request_GroupUnprocessedPosts()";
    std::cerr << std::endl;
#endif // GXSCHATS_DEBUG

    uint32_t ansType = RS_TOKREQ_ANSTYPE_DATA;
    RsTokReqOptions opts;
    opts.mReqType = GXS_REQUEST_TYPE_MSG_DATA;

    // Only Fetch UNPROCESSED messages.
    opts.mStatusFilter = GXS_SERV::GXS_MSG_STATUS_UNPROCESSED;
    opts.mStatusMask = GXS_SERV::GXS_MSG_STATUS_UNPROCESSED;

    uint32_t token = 0;

    RsGenExchange::getTokenService()->requestMsgInfo(token, ansType, opts, grouplist);
    GxsTokenQueue::queueRequest(token, GXSCHATS_UNPROCESSED_GENERIC);
}


void p3GxsChats::load_SpecificUnprocessedPosts(const uint32_t &token)
{
#ifdef GXSCHATS_DEBUG
    std::cerr << "p3GxsChats::load_SpecificUnprocessedPosts";
    std::cerr << std::endl;
#endif

    std::vector<RsGxsChatMsg> posts;
    if (!getPostData(token, posts))
    {
#ifdef GXSCHATS_DEBUG
        std::cerr << "p3GxsChats::load_SpecificUnprocessedPosts ERROR";
        std::cerr << std::endl;
#endif
        return;
    }

#ifdef GXSCHATS_DEBUG
        std::cerr << "p3GxsChats::load_SpecificUnprocessedPosts Download"<< std::endl;
        std::cerr <<"Post Size="<<posts.size()<<std::endl;
#endif

    std::vector<RsGxsChatMsg>::iterator it;
    for(it = posts.begin(); it != posts.end(); ++it)
    {
        /* autodownload the files */
#ifdef GXSCHATS_DEBUG
        std::cerr << "*********Post ID = "<<it->mMeta.mMsgId<<"  *******"<<std::endl;
#endif
        handleUnprocessedPost(*it);
    }
}


void p3GxsChats::load_GroupUnprocessedPosts(const uint32_t &token)
{
#ifdef GXSCHATS_DEBUG
    std::cerr << "p3GxsChats::load_GroupUnprocessedPosts";
    std::cerr << std::endl;
#endif

    std::vector<RsGxsChatMsg> posts;
    if (!getPostData(token, posts))
    {
#ifdef GXSCHATS_DEBUG
        std::cerr << "p3GxsChats::load_GroupUnprocessedPosts ERROR";
        std::cerr << std::endl;
#endif
        return;
    }


    std::vector<RsGxsChatMsg>::iterator it;
    for(it = posts.begin(); it != posts.end(); ++it)
    {
        handleUnprocessedPost(*it);
    }
}

void p3GxsChats::handleUnprocessedPost(const RsGxsChatMsg &msg)
{
#ifdef GXSCHATS_DEBUG
    std::cerr << "p3GxsChats::handleUnprocessedPost() GroupId: " << msg.mMeta.mGroupId << " MsgId: " << msg.mMeta.mMsgId;
    std::cerr << std::endl;
#endif

    if (!IS_MSG_UNPROCESSED(msg.mMeta.mMsgStatus))
    {
        std::cerr << "p3GxsChats::handleUnprocessedPost() Msg already Processed";
        std::cerr << std::endl;
        std::cerr << "p3GxsChats::handleUnprocessedPost() ERROR - this should not happen";
        std::cerr << std::endl;
        return;
    }

    bool enabled = false ;

    /* check that autodownload is set */
    if (autoDownloadEnabled(msg.mMeta.mGroupId,enabled) && enabled )
    {


#ifdef GXSCHATS_DEBUG
        std::cerr << "p3GxsChats::handleUnprocessedPost() AutoDownload Enabled ... handling";
        std::cerr << std::endl;
#endif

        /* check the date is not too old */
        rstime_t age = time(NULL) - msg.mMeta.mPublishTs;

        if (age < (rstime_t) CHAT_DOWNLOAD_PERIOD )
    {
        /* start download */
        // NOTE WE DON'T HANDLE PRIVATE CHANNELS HERE.
        // MORE THOUGHT HAS TO GO INTO THAT STUFF.

#ifdef GXSCHATS_DEBUG
        std::cerr << "p3GxsChats::handleUnprocessedPost() START DOWNLOAD";
        std::cerr << std::endl;
#endif

        std::list<RsGxsFile>::const_iterator fit;
        for(fit = msg.mFiles.begin(); fit != msg.mFiles.end(); ++fit)
        {
            std::string fname = fit->mName;
            Sha1CheckSum hash  = Sha1CheckSum(fit->mHash);
            uint64_t size     = fit->mSize;

            std::list<RsPeerId> srcIds;
            std::string localpath = "";
            TransferRequestFlags flags = RS_FILE_REQ_BACKGROUND | RS_FILE_REQ_ANONYMOUS_ROUTING;

            if (size < CHAT_MAX_AUTO_DL)
            {
                std::string directory ;
                if(getChannelDownloadDirectory(msg.mMeta.mGroupId,directory))
                    localpath = directory ;

                rsFiles->FileRequest(fname, hash, size, localpath, flags, srcIds);
            }
            else
                std::cerr << "WARNING: Chat file is not auto-downloaded because its size exceeds the threshold of " << CHAT_MAX_AUTO_DL << " bytes." << std::endl;
        }
    }

        /* mark as processed */
        uint32_t token;
        RsGxsGrpMsgIdPair msgId(msg.mMeta.mGroupId, msg.mMeta.mMsgId);
        setMessageProcessedStatus(token, msgId, true);
    }
#ifdef GXSCHATS_DEBUG
    else
    {
        std::cerr << "p3GxsChats::handleUnprocessedPost() AutoDownload Disabled ... skipping";
        std::cerr << std::endl;
    }
#endif
}


    // Overloaded from GxsTokenQueue for Request callbacks.
void p3GxsChats::handleResponse(uint32_t token, uint32_t req_type)
{
#ifdef GXSCHATS_DEBUG
    std::cerr << "p3GxsChats::handleResponse(" << token << "," << req_type << ")";
    std::cerr << std::endl;
#endif // GXSCHATS_DEBUG

    // stuff.
    switch(req_type)
    {
        case GXSCHATS_SUBSCRIBED_META:
            load_SubscribedGroups(token);
            break;

        case GXSCHATS_UNPROCESSED_SPECIFIC:
            load_SpecificUnprocessedPosts(token);
            break;

        case GXSCHATS_UNPROCESSED_GENERIC:
            load_SpecificUnprocessedPosts(token);
            break;

        default:
            /* error */
            std::cerr << "p3GxsService::handleResponse() Unknown Request Type: " << req_type;
            std::cerr << std::endl;
            break;
    }
}

////////////////////////////////////////////////////////////////////////////////
/// Blocking API implementation begin
////////////////////////////////////////////////////////////////////////////////

bool p3GxsChats::getChatsSummaries(
        std::list<RsGroupMetaData>& chats )
{
    uint32_t token;
    RsTokReqOptions opts;
    opts.mReqType = GXS_REQUEST_TYPE_GROUP_META;
    if( !requestGroupInfo(token, opts)
            || waitToken(token) != RsTokenService::COMPLETE ) return false;
    return getGroupSummary(token, chats);
}

bool p3GxsChats::getChatsInfo(
        const std::list<RsGxsGroupId>& chanIds,
        std::vector<RsGxsChatGroup>& chatsInfo )
{
    uint32_t token;
    RsTokReqOptions opts;
    opts.mReqType = GXS_REQUEST_TYPE_GROUP_DATA;
    if( !requestGroupInfo(token, opts, chanIds)
            || waitToken(token) != RsTokenService::COMPLETE ) return false;
    return getGroupData(token, chatsInfo);
}

bool p3GxsChats::getChatsContent(
        const std::list<RsGxsGroupId>& chanIds,
        std::vector<RsGxsChatMsg>& posts )
{
    uint32_t token;
    RsTokReqOptions opts;
    opts.mReqType = GXS_REQUEST_TYPE_MSG_DATA;
    if( !requestMsgInfo(token, opts, chanIds)
            || waitToken(token) != RsTokenService::COMPLETE ) return false;
    return getPostData(token, posts);
}

////////////////////////////////////////////////////////////////////////////////
/// Blocking API implementation end
////////////////////////////////////////////////////////////////////////////////


void p3GxsChats::setMessageProcessedStatus(uint32_t& token, const RsGxsGrpMsgIdPair& msgId, bool processed)
{
#ifdef GXSCHATS_DEBUG
    std::cerr << "p3GxsChats::setMessageProcessedStatus()";
    std::cerr << std::endl;
#endif

    uint32_t mask = GXS_SERV::GXS_MSG_STATUS_UNPROCESSED;
    uint32_t status = GXS_SERV::GXS_MSG_STATUS_UNPROCESSED;
    if (processed)
    {
        status = 0;
    }
    setMsgStatusFlags(token, msgId, status, mask);
}

void p3GxsChats::setMessageReadStatus( uint32_t& token,
                                          const RsGxsGrpMsgIdPair& msgId,
                                          bool read )
{
#ifdef GXSCHATS_DEBUG
    std::cerr << "p3GxsChats::setMessageReadStatus()";
    std::cerr << std::endl;
#endif

    /* Always remove status unprocessed */
    uint32_t mask = GXS_SERV::GXS_MSG_STATUS_GUI_NEW | GXS_SERV::GXS_MSG_STATUS_GUI_UNREAD;
    uint32_t status = GXS_SERV::GXS_MSG_STATUS_GUI_UNREAD;
    if (read) status = 0;

    setMsgStatusFlags(token, msgId, status, mask);
}


/********************************************************************************************/
/********************************************************************************************/

bool p3GxsChats::createGroup(uint32_t &token, RsGxsChatGroup &group)
{
#ifdef GXSCHATS_DEBUG
    std::cerr << "p3GxsChats::createGroup()" << std::endl;
#endif

    RsGxsChatGroupItem* grpItem = new RsGxsChatGroupItem();
    grpItem->fromChatGroup(group, true);

    RsGenExchange::publishGroup(token, grpItem);
    return true;
}


bool p3GxsChats::updateGroup(uint32_t &token, RsGxsChatGroup &group)
{
#ifdef GXSCHATS_DEBUG
    std::cerr << "p3GxsChats::updateGroup()" << std::endl;
#endif

    RsGxsChatGroupItem* grpItem = new RsGxsChatGroupItem();
    grpItem->fromChatGroup(group, true);

    RsGenExchange::updateGroup(token, grpItem);
    return true;
}

bool p3GxsChats::createGxsChatMessage(GxsNxsChatMsgItem *& mItem,  RsGxsChatMsg &msg){

    RS_STACK_MUTEX(mChatMtx);

    RsGxsChatMsgItem* msgItem = new RsGxsChatMsgItem();
    msgItem->fromChatPost(msg, true);

    RsServiceInfo  gxsChatInfo = this->getServiceInfo();

    //if we want push messages, it should be done before add GxsSync Messages (Poll synchronization).
    uint32_t size = mSerialiser->size(msgItem);
    char* mData = new char[size];
    // for fatal sign creation
    bool serialOk = mSerialiser->serialise(msgItem, mData, &size);
    if(serialOk)
    {

        RsNxsMsg* msgNxt = new RsNxsMsg(gxsChatInfo.mServiceType);

        msgNxt->grpId = msgItem->meta.mGroupId;
        //msgNxt->msgId = msgItem->meta.mMsgId;

        msgNxt->msg.setBinData(mData, size);
        // now create meta
        msgNxt->metaData = new RsGxsMsgMetaData();
        *(msgNxt->metaData) = msgItem->meta;
        // assign time stamp
        if (msgNxt->metaData){

        }

        msgNxt->metaData->mPublishTs = time(NULL);
        uint8_t createReturn = RsGenExchange::createMessage(msgNxt);

        if(createReturn == CREATE_SUCCESS){
            uint32_t size = mSerialiser->size(msgNxt);
            char* newData = new char[size];
            if(mSerialiser->serialise(msgNxt, newData, &size)){
                mItem = new GxsNxsChatMsgItem(RS_PKT_SUBTYPE_GXSCHAT_MSG);
                mItem->grpId = msgNxt->grpId;
                mItem->msg.setBinData(msgNxt->msg.bin_data,msgNxt->msg.bin_len);

                mItem->metaData = new RsGxsMsgMetaData();
                *(mItem->metaData) = *(msgNxt->metaData);
                mItem->msgId = msgNxt->msgId;
                //messageId created after the createMessage function call.

                if(mItem->metaData->mOrigMsgId.isNull())
                {
                    mItem->metaData->mOrigMsgId = mItem->metaData->mMsgId;
                }
                // now serialise meta data
                size = mItem->metaData->serial_size();

                char* metaDataBuff = new char[size];
                bool s = mItem->metaData->serialise(metaDataBuff, &size);
                s &= mItem->meta.setBinData(metaDataBuff, size);
                if (!s)
                    std::cerr << "(WW) Can't serialise or set bin data" << std::endl;

                 std::cerr << "***********createGxsChatMessage Info*************************"<<std::endl;
                 std::cerr << "MessageId:"<<mItem->msgId << " and groupId: "<<msgNxt->grpId << " and Message Size: "<<mItem->msg.TlvSize()<<std::endl;
                 std::cerr << "Mesage= "; mItem->msg.print(std::cerr, 15); std::cerr<<std::endl;
                 std::cerr <<" Meta Size:"<<mItem->meta.TlvSize()<<std::endl;
                 std::cerr <<" mAuthorId:" <<mItem->metaData->mAuthorId << std::endl;
                 std::cerr <<" mChildTs:" <<mItem->metaData->mChildTs << std::endl;
                 std::cerr <<" mGroupId:" <<mItem->metaData->mGroupId << std::endl;
                 std::cerr <<" mHash:" <<mItem->metaData->mHash << std::endl;
                 std::cerr <<" mMsgFlags:" <<mItem->metaData->mMsgFlags << std::endl;
                 std::cerr <<" mMsgId:" <<mItem->metaData->mMsgId << std::endl;
                 std::cerr <<" mMsgName:" <<mItem->metaData->mMsgName << std::endl;
                 std::cerr <<" mMsgSize:" <<mItem->metaData->mMsgSize << std::endl;
                 std::cerr <<" mMsgStatus:" <<mItem->metaData->mMsgStatus << std::endl;
                 std::cerr <<" mOrigMsgId:" <<mItem->metaData->mOrigMsgId << std::endl;
                 std::cerr <<" mParentId:" <<mItem->metaData->mParentId << std::endl;
                 std::cerr <<" mPublishTs:" <<mItem->metaData->mPublishTs << std::endl;
                 std::cerr <<" mServiceString:" <<mItem->metaData->mServiceString << std::endl;
                 std::cerr <<" mThreadId:" <<mItem->metaData->mThreadId << std::endl;
                 std::cerr <<" recvTS:" <<mItem->metaData->recvTS << std::endl;
                 std::cerr <<" refcount:" <<mItem->metaData->refcount << std::endl;
                 std::cerr <<" signSet Size:" <<mItem->metaData->signSet.TlvSize() << std::endl;
                 mItem->metaData->signSet.print(std::cerr, 15); std::cerr<<std::endl;

                //mItem->metaData->mMsgStatus = GXS_SERV::GXS_MSG_STATUS_UNPROCESSED;
                delete metaDataBuff;
                delete newData;
                return true;
            }

        }

    }
    delete []mData;
    return false;
}

void p3GxsChats::receiveNewChatMesesage(std::vector<GxsNxsChatMsgItem*>& messages){
#ifdef GXSCHATS_DEBUG
    std::cerr << "p3GxsChats::receiveNewChatMesesage()" << std::endl;
#endif

    std::vector<RsNxsMsg *>nxtMsg;
    uint32_t size =0;
    RsServiceInfo  gxsChatInfo = this->getServiceInfo();

    std::vector<GxsNxsChatMsgItem*>::iterator it;
  {
    RS_STACK_MUTEX(mChatMtx);
    for (it=messages.begin(); it != messages.end(); it++){
        GxsNxsChatMsgItem *msg = *it;

        size = mSerialiser->size(msg);
        char* mData = new char[size];
        bool serialOk = mSerialiser->serialise(msg, mData, &size);

        if (serialOk){ //converting RsChatItem to GxsMessage
            RsNxsMsg *newMsg = new RsNxsMsg(gxsChatInfo.mServiceType);
            newMsg->PeerId(msg->PeerId());

            newMsg->grpId = msg->grpId;
            newMsg->msgId = msg->msgId;
            newMsg->msg.setBinData(msg->msg.bin_data, msg->msg.bin_len);
            newMsg->meta.setBinData(msg->meta.bin_data, msg->meta.bin_len);

            std::cerr << "***********receiveNewChatMesesage() Info*************************"<<std::endl;
            std::cerr << "MessageId:"<<newMsg->msgId << " and groupId: "<<newMsg->grpId << " and Message Size: "<<newMsg->msg.TlvSize()<<std::endl;
            std::cerr << "Mesage= "; newMsg->msg.print(std::cerr, 15); std::cerr<<std::endl;
            std::cerr <<" Meta Size:"<<newMsg->meta.TlvSize()<<std::endl;
            std::cerr <<"Message Meta:"; newMsg->meta.print(std::cerr, 20); std::cerr<<std::endl;

            nxtMsg.push_back(newMsg);
            //testing publish the new message anyway.
        }
        //delete mData;
    }
  }//end mutex
  //RsGenExchange::receiveNewMessages(nxtMsg);  //send new message to validate


}
void p3GxsChats::receiveNewChatGroup(std::vector<GxsNxsChatGroupItem*>& groups){
#ifdef GXSCHATS_DEBUG
    std::cerr << "p3GxsChats::receiveNewChatGroup()" << std::endl;
#endif
}

bool p3GxsChats::createPost(uint32_t &token, RsGxsChatMsg &msg)
{
#ifdef GXSCHATS_DEBUG
    std::cerr << "p3GxsChats::createChatMsg() GroupId: " << msg.mMeta.mGroupId;
    std::cerr << std::endl;
#endif


    RsGxsChatMsgItem* msgItem = new RsGxsChatMsgItem();
    msgItem->fromChatPost(msg, true);

//    //if we want push messages, it should be done before add GxsSync Messages (Poll synchronization).

//    GxsNxsChatMsgItem* msgNxt;
//    if (createGxsChatMessage(msgNxt, msg)){
//        //std::map<RsGxsGroupId, RsGrpMetaData> mSubscribedGroups;
//        std::map<RsGxsGroupId, RsGroupMetaData>::iterator it;
//        it = mSubscribedGroups.find(msgNxt->grpId);
//        if (it != mSubscribedGroups.end()) //found group or conversation
//            sendChatMessage(msgNxt);
//    }

    RsGenExchange::publishMsg(token, msgItem);
    return true;
}

/********************************************************************************************/
/********************************************************************************************/

bool p3GxsChats::ExtraFileHash(const std::string& path)
{
    TransferRequestFlags flags = RS_FILE_REQ_ANONYMOUS_ROUTING;
    return rsFiles->ExtraFileHash(path, GXSCHAT_STOREPERIOD, flags);
}


bool p3GxsChats::ExtraFileRemove(const RsFileHash &hash)
{
    //TransferRequestFlags tflags = RS_FILE_REQ_ANONYMOUS_ROUTING | RS_FILE_REQ_EXTRA;
    RsFileHash fh = RsFileHash(hash);
    return rsFiles->ExtraFileRemove(fh);
}


/********************************************************************************************/
/********************************************************************************************/

/* so we need the same tick idea as wiki for generating dummy chats
 */

#define 	MAX_GEN_GROUPS		20
#define 	MAX_GEN_POSTS		50

std::string p3GxsChats::genRandomId()
{
    std::string randomId;
    for(int i = 0; i < 20; i++)
    {
        randomId += (char) ('a' + (RSRandom::random_u32() % 26));
    }

    return randomId;
}

bool p3GxsChats::generateDummyData()
{
    mGenCount = 0;

    std::string groupName;
    rs_sprintf(groupName, "Testchat_%d", mGenCount);

#ifdef GXSCHATS_DEBUG
    std::cerr << "p3GxsChats::generateDummyData() Starting off with Group: " << groupName;
    std::cerr << std::endl;
#endif

    /* create a new group */
    generateGroup(mGenToken, groupName);

    mGenActive = true;

    return true;
}


void p3GxsChats::dummy_tick()
{
    /* check for a new callback */

    if (mGenActive)
    {
#ifdef GXSCHATS_DEBUG
        std::cerr << "p3GxsChats::dummyTick() Gen Active";
        std::cerr << std::endl;
#endif

        uint32_t status = RsGenExchange::getTokenService()->requestStatus(mGenToken);
        if (status != RsTokenService::COMPLETE)
        {
#ifdef GXSCHATS_DEBUG
            std::cerr << "p3GxsChats::dummy_tick() Status: " << status;
            std::cerr << std::endl;
#endif

            if (status == RsTokenService::FAILED)
            {
#ifdef GXSCHATS_DEBUG
                std::cerr << "p3GxsChats::dummy_tick() generateDummyMsgs() FAILED";
                std::cerr << std::endl;
#endif
                mGenActive = false;
            }
            return;
        }

        if (mGenCount < MAX_GEN_GROUPS)
        {
            /* get the group Id */
            RsGxsGroupId groupId;
            RsGxsMessageId emptyId;
            if (!acknowledgeTokenGrp(mGenToken, groupId))
            {
                std::cerr << " ERROR ";
                std::cerr << std::endl;
                mGenActive = false;
                return;
            }

#ifdef GXSCHATS_DEBUG
            std::cerr << "p3GxsChats::dummy_tick() Acknowledged GroupId: " << groupId;
            std::cerr << std::endl;
#endif

            ChatDummyRef ref(groupId, emptyId, emptyId);
            mGenRefs[mGenCount] = ref;
        }
        else if (mGenCount < MAX_GEN_POSTS)
        {
            /* get the msg Id, and generate next snapshot */
            RsGxsGrpMsgIdPair msgId;
            if (!acknowledgeTokenMsg(mGenToken, msgId))
            {
                std::cerr << " ERROR ";
                std::cerr << std::endl;
                mGenActive = false;
                return;
            }

#ifdef GXSCHATS_DEBUG
            std::cerr << "p3GxsChats::dummy_tick() Acknowledged Post <GroupId: " << msgId.first << ", MsgId: " << msgId.second << ">";
            std::cerr << std::endl;
#endif

            /* store results for later selection */

            ChatDummyRef ref(msgId.first, mGenThreadId, msgId.second);
            mGenRefs[mGenCount] = ref;
        }

        else
        {
#ifdef GXSCHATS_DEBUG
            std::cerr << "p3GxsChats::dummy_tick() Finished";
            std::cerr << std::endl;
#endif

            /* done */
            mGenActive = false;
            return;
        }

        mGenCount++;

        if (mGenCount < MAX_GEN_GROUPS)
        {
            std::string groupName;
            rs_sprintf(groupName, "Testchat_%d", mGenCount);

#ifdef GXSCHATS_DEBUG
            std::cerr << "p3GxsChats::dummy_tick() Generating Group: " << groupName;
            std::cerr << std::endl;
#endif

            /* create a new group */
            generateGroup(mGenToken, groupName);
        }
        else if (mGenCount < MAX_GEN_POSTS)
        {
            /* create a new post */
            uint32_t idx = (uint32_t) (MAX_GEN_GROUPS * RSRandom::random_f32());
            ChatDummyRef &ref = mGenRefs[idx];

            RsGxsGroupId grpId = ref.mGroupId;
            RsGxsMessageId parentId = ref.mMsgId;
            mGenThreadId = ref.mThreadId;
            if (mGenThreadId.isNull())
            {
                mGenThreadId = parentId;
            }

#ifdef GXSCHATS_DEBUG
            std::cerr << "p3GxsChats::dummy_tick() Generating Post ... ";
            std::cerr << " GroupId: " << grpId;
            std::cerr << " ThreadId: " << mGenThreadId;
            std::cerr << " ParentId: " << parentId;
            std::cerr << std::endl;
#endif

            generatePost(mGenToken, grpId);
        }
        else
        {
            /* create a new post */
            uint32_t idx = (uint32_t) ((MAX_GEN_POSTS + 5000) * RSRandom::random_f32());
            ChatDummyRef &ref = mGenRefs[idx + MAX_GEN_POSTS];

            RsGxsGroupId grpId = ref.mGroupId;
            RsGxsMessageId parentId = ref.mMsgId;
            mGenThreadId = ref.mThreadId;
            if (mGenThreadId.isNull())
            {
                mGenThreadId = parentId;
            }


        }

    }

    cleanTimedOutSearches();
}


bool p3GxsChats::generatePost(uint32_t &token, const RsGxsGroupId &grpId)
{
    RsGxsChatMsg msg;

    std::string rndId = genRandomId();

    rs_sprintf(msg.mMsg, "chat Msg: GroupId: %s, some randomness: %s",
        grpId.toStdString().c_str(), rndId.c_str());

    msg.mMeta.mMsgName = msg.mMsg;

    msg.mMeta.mGroupId = grpId;
    msg.mMeta.mThreadId.clear() ;
    msg.mMeta.mParentId.clear() ;

    msg.mMeta.mMsgStatus = GXS_SERV::GXS_MSG_STATUS_UNPROCESSED;

    createPost(token, msg);

    return true;
}



bool p3GxsChats::generateGroup(uint32_t &token, std::string groupName)
{
    /* generate a new chat */
    RsGxsChatGroup chat;
    chat.mMeta.mGroupName = groupName;

    createGroup(token, chat);

    return true;
}


    // Overloaded from RsTickEvent for Event callbacks.
void p3GxsChats::handle_event(uint32_t event_type, const std::string &elabel)
{
#ifdef GXSCHATS_DEBUG
    std::cerr << "p3GxsChats::handle_event(" << event_type << ")";
    std::cerr << std::endl;
#endif

    // stuff.
    switch(event_type)
    {
        case CHAT_TESTEVENT_DUMMYDATA:
            generateDummyData();
            break;

        case CHAT_PROCESS:
            request_AllSubscribedGroups();
            break;

        default:
            /* error */
            std::cerr << "p3GxsChats::handle_event() Unknown Event Type: " << event_type << " elabel:" << elabel;
            std::cerr << std::endl;
            break;
    }
}

TurtleRequestId p3GxsChats::turtleGroupRequest(const RsGxsGroupId& group_id)
{
    return netService()->turtleGroupRequest(group_id) ;
}
TurtleRequestId p3GxsChats::turtleSearchRequest(const std::string& match_string)
{
    return netService()->turtleSearchRequest(match_string) ;
}

bool p3GxsChats::clearDistantSearchResults(TurtleRequestId req)
{
    return netService()->clearDistantSearchResults(req);
}
bool p3GxsChats::retrieveDistantSearchResults(TurtleRequestId req,std::map<RsGxsGroupId,RsGxsGroupSummary>& results)
{
    return netService()->retrieveDistantSearchResults(req,results);
}

bool p3GxsChats::retrieveDistantGroup(const RsGxsGroupId& group_id,RsGxsChatGroup& distant_group)
{
    RsGxsGroupSummary gs;

    if(netService()->retrieveDistantGroupSummary(group_id,gs))
    {
        // This is a placeholder information by the time we receive the full group meta data.
        distant_group.mMeta.mGroupId         = gs.mGroupId ;
        distant_group.mMeta.mGroupName       = gs.mGroupName;
        distant_group.mMeta.mGroupFlags      = GXS_SERV::FLAG_PRIVACY_PUBLIC ;
        distant_group.mMeta.mSignFlags       = gs.mSignFlags;

        distant_group.mMeta.mPublishTs       = gs.mPublishTs;
        distant_group.mMeta.mAuthorId        = gs.mAuthorId;

        distant_group.mMeta.mCircleType      = GXS_CIRCLE_TYPE_PUBLIC ;// guessed, otherwise the group would not be search-able.

        // other stuff.
        distant_group.mMeta.mAuthenFlags     = 0;	// wild guess...

        distant_group.mMeta.mSubscribeFlags  = GXS_SERV::GROUP_SUBSCRIBE_NOT_SUBSCRIBED ;

        distant_group.mMeta.mPop             = gs.mPopularity; 			// Popularity = number of friend subscribers
        distant_group.mMeta.mVisibleMsgCount = gs.mNumberOfMessages; 	// Max messages reported by friends
        distant_group.mMeta.mLastPost        = gs.mLastMessageTs; 		// Timestamp for last message. Not used yet.

        return true ;
    }
    else
        return false ;
}

bool p3GxsChats::turtleSearchRequest(
        const std::string& matchString,
        const std::function<void (const RsGxsGroupSummary&)>& multiCallback,
        rstime_t maxWait )
{
    if(matchString.empty())
    {
        std::cerr << __PRETTY_FUNCTION__ << " match string can't be empty!"
                  << std::endl;
        return false;
    }

    TurtleRequestId sId = turtleSearchRequest(matchString);

    RS_STACK_MUTEX(mSearchCallbacksMapMutex);
    mSearchCallbacksMap.emplace(
                sId,
                std::make_pair(
                    multiCallback,
                    std::chrono::system_clock::now() +
                        std::chrono::seconds(maxWait) ) );

    return true;
}

void p3GxsChats::receiveDistantSearchResults(
        TurtleRequestId id, const RsGxsGroupId& grpId )
{
    std::cerr << __PRETTY_FUNCTION__ << "(" << id << ", " << grpId << ")"
              << std::endl;

    RsGenExchange::receiveDistantSearchResults(id, grpId);
    RsGxsGroupSummary gs;
    gs.mGroupId = grpId;
    netService()->retrieveDistantGroupSummary(grpId, gs);

    {
        RS_STACK_MUTEX(mSearchCallbacksMapMutex);
        auto cbpt = mSearchCallbacksMap.find(id);
        if(cbpt != mSearchCallbacksMap.end())
            cbpt->second.first(gs);
    } // end RS_STACK_MUTEX(mSearchCallbacksMapMutex);
}

void p3GxsChats::cleanTimedOutSearches()
{
    RS_STACK_MUTEX(mSearchCallbacksMapMutex);
    auto now = std::chrono::system_clock::now();
    for( auto cbpt = mSearchCallbacksMap.begin();
         cbpt != mSearchCallbacksMap.end(); )
        if(cbpt->second.second <= now)
        {
            clearDistantSearchResults(cbpt->first);
            cbpt = mSearchCallbacksMap.erase(cbpt);
        }
        else ++cbpt;
}

/**  Download Functions ***/
/********************************************************************************************/
/********************************************************************************************/

#if 0
bool p3GxsChats::setChannelAutoDownload(uint32_t &token, const RsGxsGroupId &groupId, bool autoDownload)
{
    std::cerr << "p3GxsChats::setChannelAutoDownload()";
    std::cerr << std::endl;

    // we don't actually use the token at this point....
    bool p3GxsChats::setAutoDownload(const RsGxsGroupId &groupId, bool enabled)



    return;
}
#endif

bool SSGxsChatGroup::load(const std::string &input)
{
    if(input.empty())
    {
#ifdef GXSCHATS_DEBUG
        std::cerr << "SSGxsChatGroup::load() asked to load a null string." << std::endl;
#endif
        return true ;
    }
    int download_val;
    mAutoDownload = false;
    mDownloadDirectory.clear();


    RsTemporaryMemory tmpmem(input.length());

    if (1 == sscanf(input.c_str(), "D:%d", &download_val))
    {
        if (download_val == 1)
            mAutoDownload = true;
    }
    else  if( 2 == sscanf(input.c_str(),"v2 {D:%d} {P:%[^}]}",&download_val,(unsigned char*)tmpmem))
    {
        if (download_val == 1)
            mAutoDownload = true;

        std::vector<uint8_t> vals = Radix64::decode(std::string((char*)(unsigned char *)tmpmem)) ;
        mDownloadDirectory = std::string((char*)vals.data(),vals.size());
    }
    else  if( 1 == sscanf(input.c_str(),"v2 {D:%d}",&download_val))
    {
        if (download_val == 1)
            mAutoDownload = true;
    }
    else
    {
#ifdef GXSCHATS_DEBUG
        std::cerr << "SSGxsChatGroup::load(): could not parse string \"" << input << "\"" << std::endl;
#endif
        return false ;
    }

#ifdef GXSCHATS_DEBUG
    std::cerr << "DECODED STRING: autoDL=" << mAutoDownload << ", directory=\"" << mDownloadDirectory << "\"" << std::endl;
#endif

    return true;
}

std::string SSGxsChatGroup::save() const
{
    std::string output = "v2 ";

    if (mAutoDownload)
        output += "{D:1}";
    else
        output += "{D:0}";

    if(!mDownloadDirectory.empty())
    {
        std::string encoded_str ;
        Radix64::encode((unsigned char*)mDownloadDirectory.c_str(),mDownloadDirectory.length(),encoded_str);

        output += " {P:" + encoded_str + "}";
    }

#ifdef GXSCHATS_DEBUG
    std::cerr << "ENCODED STRING: " << output << std::endl;
#endif

    return output;
}

bool p3GxsChats::autoDownloadEnabled(const RsGxsGroupId &groupId,bool& enabled)
{
#ifdef GXSCHATS_DEBUG
    std::cerr << "p3GxsChats::autoDownloadEnabled(" << groupId << ")";
    std::cerr << std::endl;
#endif

    std::map<RsGxsGroupId, RsGroupMetaData>::iterator it;

    it = mSubscribedGroups.find(groupId);
    if (it == mSubscribedGroups.end())
    {
#ifdef GXSCHATS_DEBUG
        std::cerr << "p3GxsChats::autoDownloadEnabled() No Entry";
        std::cerr << std::endl;
#endif

        return false;
    }

    /* extract from ServiceString */
    SSGxsChatGroup ss;
    ss.load(it->second.mServiceString);
    enabled = ss.mAutoDownload;

#ifdef GXSCHATS_DEBUG
    std::cerr << "p3GxsChats::autoDownloadEnabled.mServiceString =" << it->second.mServiceString << " and enabled="<<enabled;
    std::cerr << std::endl;
#endif
    return true;
}

bool p3GxsChats::setAutoDownload(const RsGxsGroupId &groupId, bool enabled)
{
#ifdef GXSCHATS_DEBUG
    std::cerr << "p3GxsChats::setAutoDownload() id: " << groupId << " enabled: " << enabled;
    std::cerr << std::endl;
#endif

    std::map<RsGxsGroupId, RsGroupMetaData>::iterator it;

    it = mSubscribedGroups.find(groupId);
    if (it == mSubscribedGroups.end())
    {
#ifdef GXSCHATS_DEBUG
        std::cerr << "p3GxsChats::setAutoDownload() Missing Group";
        std::cerr << std::endl;
#endif

        return false;
    }

    /* extract from ServiceString */
    SSGxsChatGroup ss;
    ss.load(it->second.mServiceString);
    if (enabled == ss.mAutoDownload)
    {
        /* it should be okay! */
#ifdef GXSCHATS_DEBUG
        std::cerr << "p3GxsChats::setAutoDownload() WARNING setting looks okay already";
        std::cerr << std::endl;
#endif

    }

    /* we are just going to set it anyway. */
    ss.mAutoDownload = enabled;
    std::string serviceString = ss.save();
    uint32_t token;

    it->second.mServiceString = serviceString; // update Local Cache.
    RsGenExchange::setGroupServiceString(token, groupId, serviceString); // update dbase.

    /* now reload it */
    std::list<RsGxsGroupId> groups;
    groups.push_back(groupId);

    request_SpecificSubscribedGroups(groups);

    return true;
}

bool p3GxsChats::setChannelAutoDownload(const RsGxsGroupId &groupId, bool enabled)
{
    return setAutoDownload(groupId, enabled);
}


bool p3GxsChats::getChannelAutoDownload(const RsGxsGroupId &groupId, bool& enabled)
{
    return autoDownloadEnabled(groupId,enabled);
}

bool p3GxsChats::setChannelDownloadDirectory(const RsGxsGroupId &groupId, const std::string& directory)
{
#ifdef GXSCHATS_DEBUG
    std::cerr << "p3GxsChats::setDownloadDirectory() id: " << groupId << " to: " << directory << std::endl;
#endif

    std::map<RsGxsGroupId, RsGroupMetaData>::iterator it;

    it = mSubscribedGroups.find(groupId);
    if (it == mSubscribedGroups.end())
    {
#ifdef GXSCHATS_DEBUG
        std::cerr << "p3GxsChats::setAutoDownload() Missing Group" << std::endl;
#endif
        return false;
    }

    /* extract from ServiceString */
    SSGxsChatGroup ss;
    ss.load(it->second.mServiceString);

    if (directory == ss.mDownloadDirectory)
    {
#ifdef GXSCHATS_DEBUG
        std::cerr << "p3GxsChats::setDownloadDirectory() WARNING setting looks okay already" << std::endl;
#endif

    }

    /* we are just going to set it anyway. */
    ss.mDownloadDirectory = directory;
    std::string serviceString = ss.save();
    uint32_t token;

    it->second.mServiceString = serviceString; // update Local Cache.
    RsGenExchange::setGroupServiceString(token, groupId, serviceString); // update dbase.

    /* now reload it */
    std::list<RsGxsGroupId> groups;
    groups.push_back(groupId);

    request_SpecificSubscribedGroups(groups);

    return true;
}

bool p3GxsChats::getChannelDownloadDirectory(const RsGxsGroupId & groupId,std::string& directory)
{
#ifdef GXSCHATS_DEBUG
    std::cerr << "p3GxsChats::getChannelDownloadDirectory(" << groupId << ")" << std::endl;
#endif

    std::map<RsGxsGroupId, RsGroupMetaData>::iterator it;

    it = mSubscribedGroups.find(groupId);

    if (it == mSubscribedGroups.end())
    {
#ifdef GXSCHATS_DEBUG
        std::cerr << "p3GxsChats::getChannelDownloadDirectory() No Entry" << std::endl;
#endif

        return false;
    }

    /* extract from ServiceString */
    SSGxsChatGroup ss;
    ss.load(it->second.mServiceString);
    directory = ss.mDownloadDirectory;

    return true ;
}
