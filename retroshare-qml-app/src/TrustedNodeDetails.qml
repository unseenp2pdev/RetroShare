/*
 * RetroShare Android QML App
 * Copyright (C) 2017  Gioacchino Mazzurco <gio@eigenlab.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

import QtQuick 2.7
import QtQuick.Controls 2.0
import "components/."

Item
{
	id: nodeDetailsRoot

	property string pgpName
	property alias pgpId: pgpIdTxt.text
	property bool isOnline

	property string nodeCert

	property var locations

	function attemptConnectionCB (par)
	{
		console.log("attemptConnectionCB()", par.response)
	}


	Image
	{
		id: nodeStatusImage
		source: isOnline?
					"icons/state-ok.svg" :
					"icons/state-offline.svg"

		height: 128
		sourceSize.height: height
		fillMode: Image.PreserveAspectFit

		anchors.top: parent.top
		anchors.topMargin: 6
		anchors.horizontalCenter: parent.horizontalCenter
	}



	Column
	{
		id: pgpColumn

		anchors.top: nodeStatusImage.bottom
		width: parent.width

		Text
		{
            text: nodeDetailsRoot.pgpName.replace(" (Generated by UnseenP2P) <>", "")
			anchors.horizontalCenter: parent.horizontalCenter
			font.pixelSize: 20
		}
		Text
		{
			id: pgpIdTxt
			anchors.horizontalCenter: parent.horizontalCenter
			color: "darkslategrey"
		}
	}

	JSONListModel
	{
		id: jsonModel
		json: JSON.stringify(nodeDetailsRoot.locations)
	}

	ListView
	{
		width: parent.width * .75
		anchors.top: pgpColumn.bottom
		anchors.topMargin: 5
		anchors.bottom: buttonsRow.top
		model: jsonModel.model
		anchors.horizontalCenter: parent.horizontalCenter
		headerPositioning: ListView.OverlayHeader

		clip: true
		snapMode: ListView.SnapToItem

		spacing:7

		header:Rectangle
		{
			color: "aliceblue"
			width: parent.width
			z: 2
			height: headetText.contentHeight + 10
			radius: 10

			Text
			{
				id: headetText
				text: "Node locations ("+jsonModel.model.count+")"
				anchors.horizontalCenter: parent.horizontalCenter
				anchors.verticalCenter: parent.verticalCenter
				font.italic: true
			}

		}

		delegate:
		MouseArea
		{
			width: parent.width
			height: innerCol.height

			onClicked:
			{
				console.log("triggerLocationConnectionAttempt()")
				rsApi.request("/peers/attempt_connection/",
							  JSON.stringify({peer_id: model.peer_id}) , attemptConnectionCB)

			}

			Column
			{
				id: innerCol
				height: idRow.height + gxsInfo.height
				width: parent.width
				leftPadding: 4
				spacing: 6

				Row
				{
					id: idRow
					height: 30
					spacing: 4

					Image
					{
						id: statusImage
						source: model.is_online ?
									"icons/network-connect.svg" :
									"icons/network-disconnect.svg"

						height: parent.height - 4
						sourceSize.height: height
						fillMode: Image.PreserveAspectFit
						anchors.verticalCenter: parent.verticalCenter
					}
					Text
					{
						id: locNameText
						text: model.location
						anchors.verticalCenter: parent.verticalCenter
					}
				}

				TextAndIcon
				{
					id: gxsInfo
					width: parent.width
					innerText:  model.peer_id
					anchors.horizontalCenter: parent.horizontalCenter
					iconUrl:  "/icons/keyring.svg"

				}
			}
		}
	}

	Row
	{
		id: buttonsRow

		anchors.bottom: parent.bottom
		anchors.horizontalCenter: parent.horizontalCenter
		spacing: 6

		ButtonText
		{
			text: qsTr("Revoke")

			borderRadius: 0
			buttonTextPixelSize: 14
			iconUrl: "/icons/leave.svg"

			onClicked:
				rsApi.request(
					"/peers/"+nodeDetailsRoot.pgpId+"/delete", "",
					function()
					{ stackView.push("qrc:/TrustedNodesView.qml") })
		}

		ButtonText
		{
			text: qsTr("Entrust")

			visible: nodeDetailsRoot.nodeCert.length > 0

			borderRadius: 0
			buttonTextPixelSize: 14
			iconUrl: "/icons/invite.svg"

			onClicked:
			{
				var jsonData =
				{
					cert_string: nodeCert,
					flags:
					{
						allow_direct_download: true,
						allow_push: false,
						require_whitelist: false,
					}
				}
				rsApi.request(
							"PUT /peers", JSON.stringify(jsonData),
							function()
							{ stackView.push("qrc:/TrustedNodesView.qml") })
			}
		}
	}
}
