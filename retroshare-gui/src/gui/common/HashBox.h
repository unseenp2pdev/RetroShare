/****************************************************************
 * This file is distributed under the following license:
 *
 * Copyright (c) 2011, RetroShare Team
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

#ifndef HASHBOX_H
#define HASHBOX_H

#include <QScrollArea>
#include <stdint.h>
#include <retroshare/rsfiles.h>

namespace Ui {
	class HashBox;
}

class AttachFileItem;
class QVBoxLayout;
class SubFileItem; //unseenp2p - use gxs file in the chatWidget file sharing

class HashedFile
{
public:
	enum Flags {
		NoFlag   = 0,
		Picture  = 1
	};

public:
	QString filename;
	QString filepath;
	uint64_t size;
    RsFileHash hash;
	Flags flag;

public:
	HashedFile();
};

class HashBox : public QScrollArea
{
	Q_OBJECT

public:
	explicit HashBox(QWidget *parent = 0);
	~HashBox();

	void setAutoHide(bool autoHide);
	void addAttachments(const QStringList& files,TransferRequestFlags tfl, HashedFile::Flags flag = HashedFile::NoFlag);

    //unseenp2p: add gxsfile
    void addGxsFileAttachments(const QStringList& files,TransferRequestFlags tfl, HashedFile::Flags flag = HashedFile::NoFlag);
    //void addAttachmentGxsFilePath(const std::string &path);

	void setDropWidget(QWidget* widget);
	void setDefaultTransferRequestFlags(TransferRequestFlags flags) { mDefaultTransferFlags = flags ; }

protected:
	bool eventFilter(QObject *object, QEvent *event);

private slots:
	void fileFinished(AttachFileItem* file);
	void checkAttachmentReady();
    void checkGxsFileAttachmentReady();

    void gxsFileFinished(SubFileItem* file);
signals:
	void fileHashingStarted();
	void fileHashingFinished(QList<HashedFile> hashedFiles);
    //void gxsfileHashingFinishedForGUI(QList<HashedFile> hashedFiles, std::list<SubFileItem *> mFiles);
    void gxsfileHashingFinishedForGUI(QList<HashedFile> hashedFiles);
//    void gxsfileHashingFinished(std::list<SubFileItem *> mFiles);

private:
	class HashingInfo
	{
	public:
		AttachFileItem* item;
		HashedFile::Flags flag;
	};

    class GxsFileHashingInfo
    {
    public:
        SubFileItem* item;
        HashedFile::Flags flag;
    };

	QList<HashingInfo> mHashingInfos;
    QList<GxsFileHashingInfo> mGxsFileHashingInfos; //unseenp2p - using for gxs file sharing in ChatWidget

    //std::list<SubFileItem *> mAttachments; //unseenp2p
    QList<SubFileItem *> mAttachments; //unseenp2p

	bool mAutoHide;
	QWidget* dropWidget;
	Ui::HashBox *ui;
	TransferRequestFlags mDefaultTransferFlags ;
};

#endif // HASHBOX_H
