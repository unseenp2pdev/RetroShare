/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the FOO module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

function Component()
{
    // default constructor
}

Component.prototype.createOperations = function()
{
    // call default implementation to actually install README.txt!
    component.createOperations();
    var targetDir = installer.value("TargetDir");
    var homeDir = installer.value("HomeDir"); 
    
    if (systemInfo.productType === "windows") {
        component.addOperation("CreateShortcut", "@TargetDir@/unseenp2p.exe", "@StartMenuDir@/Unseen Communicator");
    }
    else if (systemInfo.productType === "osx") {
	maintenanceToolPath = installer.value("TargetDir") + "/MaintenanceTool";
    }
    else {
	var desktopFileTarget = installer.value("HomeDir") + "/.local/share/applications";

        component.addOperation("CreateDesktopEntry", 
                                  targetDir + "/unseenp2p.desktop", 
                                  "Type=Application\nTerminal=false\nExec= " + targetDir + "/unseenp2p.app\nName=unseenp2p\nIcon= " + targetDir + "/unseenp2p.xpm");

    	component.addOperation("CreateDesktopEntry", 
                                  homeDir + "/Desktop/unseenp2p.desktop", 
                                  "Type=Application\nTerminal=false\nExec= " + targetDir + "/unseenp2p.app\nName=unseenp2p\nIcon= " + targetDir + "/unseenp2p.xpm");

	// in case autostart is not exist.
	component.addOperation("Mkdir", homeDir + "/.config/autostart");

    	component.addOperation("CreateDesktopEntry", 
                                  homeDir + "/.config/autostart/unseenp2p.desktop", 
                                  "Type=Application\nTerminal=false\nExec= " + targetDir + "/unseenp2p.app\nName=unseenp2p\nIcon= " + targetDir + "/unseenp2p.xpm");

    }
}
