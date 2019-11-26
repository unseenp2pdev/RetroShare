/****************************************************************
 * This file is distributed under the following license:
 *
 * Copyright (c) 2010, RetroShare Team
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, 
 *  Boston, MA  02110-1301, USA.
 ****************************************************************/

#include <QPixmap>
#include <QImage>
#include <QSize>
#include <QPainter>

#include <retroshare/rsmsgs.h>
#include <retroshare/rspeers.h>
#include <retroshare/rsidentity.h>
#include <gui/gxs/GxsIdDetails.h>

#include "AvatarDefs.h"

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

void AvatarDefs::getOwnAvatar(QPixmap &avatar, const QString& defaultImage)
{
	unsigned char *data = NULL;
	int size = 0;

	/* get avatar */
	rsMsgs->getOwnAvatarData(data, size);

	if (size == 0) {
		avatar = QPixmap(defaultImage);
		return;
	}

	/* load image */
	avatar.loadFromData(data, size, "PNG") ;

	free(data);
}
void AvatarDefs::getAvatarFromSslId(const RsPeerId& sslId, QPixmap &avatar, const QString& defaultImage)
{
    unsigned char *data = NULL;
    int size = 0;

    /* get avatar */
    rsMsgs->getAvatarData(RsPeerId(sslId), data, size);
    if (size == 0) {
        avatar = QPixmap(defaultImage);
        return;
    }

    /* load image */
    avatar.loadFromData(data, size, "PNG") ;

    free(data);
}
void AvatarDefs::getAvatarFromGxsId(const RsGxsId& gxsId, QPixmap &avatar, const QString& defaultImage)
{
    //int size = 0;

    /* get avatar */
    RsIdentityDetails details ;

    if(!rsIdentity->getIdDetails(gxsId, details))
    {
        avatar = QPixmap(defaultImage);
        return ;
    }

    /* load image */

        if(details.mAvatar.mSize == 0 || !avatar.loadFromData(details.mAvatar.mData, details.mAvatar.mSize, "PNG"))
        {
            //unseenp2p - make the circle avatar
            QImage circleAvatar = GxsIdDetails::makeDefaultIcon(gxsId);
            avatar = QPixmap::fromImage(getCirclePhoto(circleAvatar, circleAvatar.size().width()));
        }
}

void AvatarDefs::getAvatarFromGpgId(const RsPgpId& gpgId, QPixmap &avatar, const QString& defaultImage)
{
	unsigned char *data = NULL;
	int size = 0;

    if (gpgId == rsPeers->getGPGOwnId()) {
		/* Its me */
		rsMsgs->getOwnAvatarData(data,size);
	} else {
		/* get the first available avatar of one of the ssl ids */
        std::list<RsPeerId> sslIds;
        if (rsPeers->getAssociatedSSLIds(gpgId, sslIds)) {
            std::list<RsPeerId>::iterator sslId;
			for (sslId = sslIds.begin(); sslId != sslIds.end(); ++sslId) {
				rsMsgs->getAvatarData(*sslId, data, size);
				if (size) {
					break;
				}
			}
		}
	}

	if (size == 0) {
		avatar = QPixmap(defaultImage);
		return;
	}

	/* load image */
	avatar.loadFromData(data, size, "PNG") ;

	free(data);
}
