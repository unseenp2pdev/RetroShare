#ifndef RSGXSCHATS_H
#define RSGXSCHATS_H

/*******************************************************************************
 * libretroshare/src/retroshare: rsgxschats.h                               *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2012-2012 by Robert Fernie <retroshare@lunamutt.com>              *
 * Copyright (C) 2018  Gioacchino Mazzurco <gio@eigenlab.org>                  *
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

#include <inttypes.h>
#include <string>
#include <list>
#include <functional>

#include "retroshare/rstokenservice.h"
#include "retroshare/rsgxsifacehelper.h"
#include "retroshare/rsgxscommon.h"
#include "serialiser/rsserializable.h"
#include "retroshare/rsturtle.h"

class RsGxsChats;

/**
 * Pointer to global instance of RsGxschats service implementation
 * @jsonapi{development}
 */
extern RsGxsChats *rsGxsChats;

class RsGxsChatGroup : RsSerializable
{
    public:
        RsGroupMetaData mMeta;
        std::string mDescription;   //conversation display name or groupname
        RsGxsImage  mImage;         //conversation avatar image
        /// @see RsSerializable
        virtual void serial_process( RsGenericSerializer::SerializeJob j,
                                 RsGenericSerializer::SerializeContext& ctx )
        {
            RS_SERIAL_PROCESS(mMeta);
            RS_SERIAL_PROCESS(mImage);
            RS_SERIAL_PROCESS(mDescription);
        }
};

std::ostream &operator<<(std::ostream& out, const RsGxsChatGroup& group);

class RsGxsChatMsg : RsSerializable
{
    public:
    RsMsgMetaData mMeta;
    std::string mMsg;               //text message
    std::list<RsGxsFile> mFiles;    //file attachment
    RsGxsImage mThumbnail;          //photo sharing

    /// @see RsSerializable
    virtual void serial_process( RsGenericSerializer::SerializeJob j,
                                 RsGenericSerializer::SerializeContext& ctx )
    {
        RS_SERIAL_PROCESS(mMeta);
        RS_SERIAL_PROCESS(mMsg);
        RS_SERIAL_PROCESS(mFiles);
        RS_SERIAL_PROCESS(mThumbnail);
    }
};

std::ostream &operator<<(std::ostream& out, const RsGxsChatMsg& post);

class RsGxsChats: public RsGxsIfaceHelper
{
public:

    explicit RsGxsChats(RsGxsIface& gxs) : RsGxsIfaceHelper(gxs) {}
    virtual ~RsGxsChats() {}

    /**
     * @brief Get chats summaries list. Blocking API.
     * @jsonapi{development}
     * @param[out] chats list where to store the chats
     * @return false if something failed, true otherwhise
     */
    virtual bool getChatsSummaries(std::list<RsGroupMetaData>& chats) = 0;

    /**
     * @brief Get chats information (description, thumbnail...).
     * Blocking API.
     * @jsonapi{development}
     * @param[in] chanIds ids of the chats of which to get the informations
     * @param[out] chatsInfo storage for the chats informations
     * @return false if something failed, true otherwhise
     */
    virtual bool getChatsInfo(
            const std::list<RsGxsGroupId>& chanIds,
            std::vector<RsGxsChatGroup>& chatsInfo ) = 0;

    /**
     * @brief Get content of specified chats. Blocking API
     * @jsonapi{development}
     * @param[in] chanIds id of the chats of which the content is requested
     * @param[out] posts storage for the posts
     * @param[out] comments storage for the comments
     * @return false if something failed, true otherwhise
     */
    virtual bool getChatsContent(
            const std::list<RsGxsGroupId>& chatIds,
            std::vector<RsGxsChatMsg>& posts) = 0;

    /* Specific Service Data
     * TODO: change the orrible const uint32_t &token to uint32_t token
     * TODO: create a new typedef for token so code is easier to read
     */

    virtual bool getGroupData(const uint32_t &token, std::vector<RsGxsChatGroup> &groups) = 0;
    virtual bool getPostData(const uint32_t &token, std::vector<RsGxsChatMsg> &posts, std::vector<RsGxsComment> &cmts) = 0;
    virtual bool getPostData(const uint32_t &token, std::vector<RsGxsChatMsg> &posts) = 0;

    /**
     * @brief toggle message read status
     * @jsonapi{development}
     * @param[out] token GXS token queue token
     * @param[in] msgId
     * @param[in] read
     */
    virtual void setMessageReadStatus(
            uint32_t& token, const RsGxsGrpMsgIdPair& msgId, bool read) = 0;


    /**
     * @brief Share chat publishing key
     * This can be used to authorize other peers to post on the chat
     * @jsonapi{development}
     * param[in] groupId chat id
     * param[in] peers peers to which share the key
     * @return false on error, true otherwise
     */
    virtual bool groupShareKeys(
            const RsGxsGroupId& groupId, const std::set<RsPeerId>& peers ) = 0;

    /**
     * @brief Request subscription to a group.
     * The action is performed asyncronously, so it could fail in a subsequent
     * phase even after returning true.
     * @jsonapi{development}
     * @param[out] token Storage for RsTokenService token to track request
     * status.
     * @param[in] groupId chat id
     * @param[in] subscribe
     * @return false on error, true otherwise
     */
    virtual bool subscribeToGroup( uint32_t& token, const RsGxsGroupId &groupId,
                                   bool subscribe ) = 0;

    /**
     * @brief Request chat creation.
     * The action is performed asyncronously, so it could fail in a subsequent
     * phase even after returning true.
     * @jsonapi{development}
     * @param[out] token Storage for RsTokenService token to track request
     * status.
     * @param[in] group chat data (name, description...)
     * @return false on error, true otherwise
     */
    virtual bool createGroup(uint32_t& token, RsGxsChatGroup& group) = 0;

    /**
     * @brief Request post creation.
     * The action is performed asyncronously, so it could fail in a subsequent
     * phase even after returning true.
     * @jsonapi{development}
     * @param[out] token Storage for RsTokenService token to track request
     * status.
     * @param[in] post
     * @return false on error, true otherwise
     */
    virtual bool createPost(uint32_t& token, RsGxsChatMsg& post) = 0;

    /**
     * @brief Request chat change.
     * The action is performed asyncronously, so it could fail in a subsequent
     * phase even after returning true.
     * @jsonapi{development}
     * @param[out] token Storage for RsTokenService token to track request
     * status.
     * @param[in] group chat data (name, description...) with modifications
     * @return false on error, true otherwise
     */
    virtual bool updateGroup(uint32_t& token, RsGxsChatGroup& group) = 0;

    /**
     * @brief Share extra file
     * Can be used to share extra file attached to a chat post
     * @jsonapi{development}
     * @param[in] path file path
     * @return false on error, true otherwise
     */
    virtual bool ExtraFileHash(const std::string& path) = 0;

    /**
     * @brief Remove extra file from shared files
     * @jsonapi{development}
     * @param[in] hash hash of the file to remove
     * @return false on error, true otherwise
     */
    virtual bool ExtraFileRemove(const RsFileHash& hash) = 0;

    /**
     * @brief Request remote chats search
     * @jsonapi{development}
     * @param[in] matchString string to look for in the search
     * @param multiCallback function that will be called each time a search
     * result is received
     * @param[in] maxWait maximum wait time in seconds for search results
     * @return false on error, true otherwise
     */
    virtual bool turtleSearchRequest(
            const std::string& matchString,
            const std::function<void (const RsGxsGroupSummary& result)>& multiCallback,
            rstime_t maxWait = 300 ) = 0;

    //////////////////////////////////////////////////////////////////////////////
    ///                     Distant synchronisation methods                    ///
    //////////////////////////////////////////////////////////////////////////////
    ///
    virtual TurtleRequestId turtleGroupRequest(const RsGxsGroupId& group_id)=0;
    virtual TurtleRequestId turtleSearchRequest(const std::string& match_string)=0;
    virtual bool retrieveDistantSearchResults(TurtleRequestId req, std::map<RsGxsGroupId, RsGxsGroupSummary> &results) =0;
    virtual bool clearDistantSearchResults(TurtleRequestId req)=0;
    virtual bool retrieveDistantGroup(const RsGxsGroupId& group_id,RsGxsChatGroup& distant_group)=0;

    //////////////////////////////////////////////////////////////////////////////
};


#endif // RSGXSCHATS_H
