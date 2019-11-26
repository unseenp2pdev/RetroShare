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
    installer.gainAdminRights();
    component.loaded.connect(this, this.installerLoaded);
}

Component.prototype.createOperations = function()
{
    // call default implementation to actually install README.txt!
    component.createOperations();
    var targetDir = installer.value("TargetDir");
    var homeDir = installer.value("HomeDir"); 
    
    if (systemInfo.productType === "windows") {
        component.addOperation("CreateShortcut", "@TargetDir@/unseenp2p.exe", "@DesktopDir@/Unseen Communicator.lnk","workingDirectory=@TargetDir@");
        component.addOperation("CreateShortcut", "@TargetDir@/unseenp2p.exe", "@StartMenuDir@/Unseen Communicator.lnk","workingDirectory=@TargetDir@");
    }
    else if (systemInfo.productType === "osx") {
	maintenanceToolPath = installer.value("TargetDir") + "/MaintenanceTool";
        var users = installer.value("HomeDir").split("\/");
        var targetDir = installer.value("TargetDir");
        component.addOperation("Execute", "sudo", ["chown", "-R", users[2], targetDir]);

    }
    else {
	var desktopFileTarget = installer.value("HomeDir") + "/.local/share/applications";

        component.addOperation("CreateDesktopEntry", 
                                  targetDir + "/unseenp2p.desktop", 
                                  "Type=Application\nTerminal=false\nExec=" + targetDir + "/unseenp2p.app\nName=unseenp2p\nIcon=" + targetDir + "/unseenp2p.xpm");

    	component.addOperation("CreateDesktopEntry", 
                                  homeDir + "/Desktop/unseenp2p.desktop", 
                                  "Type=Application\nTerminal=false\nExec=" + targetDir + "/unseenp2p.app\nName=unseenp2p\nIcon=" + targetDir + "/unseenp2p.xpm");

	// in case autostart is not exist.
	component.addOperation("Mkdir", homeDir + "/.config/autostart");

    	component.addOperation("CreateDesktopEntry", 
                                  homeDir + "/.config/autostart/unseenp2p.desktop", 
                                  "Type=Application\nTerminal=false\nExec=" + targetDir + "/unseenp2p.app\nName=unseenp2p\nIcon=" + targetDir + "/unseenp2p.xpm");

	var users = installer.value("HomeDir").split("\/");

	component.addOperation("Execute", "sudo", ["chown", users[2] + ":" + users[2], homeDir + "/.config/autostart"]);
	component.addOperation("Execute", "sudo", ["chown", users[2] + ":" + users[2], homeDir + "/Desktop/unseenp2p.desktop"]);
	component.addOperation("Execute", "sudo", ["chown", users[2] + ":" + users[2], targetDir + "/unseenp2p.desktop"]);
	component.addOperation("Execute", "sudo", ["cp", targetDir + "/unseenp2p.desktop", "/usr/share/applications/unseenp2p.desktop"]);
	
	
    }
}

Component.prototype.installerLoaded = function()
{

    installer.setDefaultPageVisible(QInstaller.TargetDirectory, false);
    installer.addWizardPage(component, "TargetWidget", QInstaller.TargetDirectory);

    targetDirectoryPage = gui.pageWidgetByObjectName("DynamicTargetWidget");
    targetDirectoryPage.windowTitle = "Choose Installation Directory";
    targetDirectoryPage.description.setText("Please select where the UnseenP2P Communicator will be installed:");
    targetDirectoryPage.targetDirectory.textChanged.connect(this, this.targetDirectoryChanged);
    targetDirectoryPage.targetDirectory.setText(installer.value("TargetDir"));
    targetDirectoryPage.targetChooser.released.connect(this, this.targetChooserClicked);

    gui.pageById(QInstaller.ComponentSelection).entered.connect(this, this.componentSelectionPageEntered);
    //gui.pageById(QInstaller.PerformInstallation).entered.connect(this, this.componentPerformInstallationPageEntered);
}

Component.prototype.componentPerformInstallationPageEntered = function ()
{
    // before we install, we'll run the uninstaller silently (not shown)
}

Component.prototype.targetChooserClicked = function()
{
    var dir = QFileDialog.getExistingDirectory("", targetDirectoryPage.targetDirectory.text);
    targetDirectoryPage.targetDirectory.setText(dir);
}


Component.prototype.targetDirectoryChanged = function()
{
    var dir = targetDirectoryPage.targetDirectory.text;
    if (installer.fileExists(dir) && (installer.fileExists(dir + "/maintenancetool.exe") || installer.fileExists(dir + "/maintenancetool.app") || installer.fileExists(dir + "/maintenancetool") ) ) {
        targetDirectoryPage.warning.setText("<p style=\"color: red\">Existing installation detected and will be overwritten.</p>");
    }
    else if (installer.fileExists(dir)) {
        targetDirectoryPage.warning.setText("<p style=\"color: red\">Installing in existing directory. It will be wiped on uninstallation.</p>");
    }
    else {
        targetDirectoryPage.warning.setText("");
    }
    installer.setValue("TargetDir", dir);
}


Component.prototype.componentSelectionPageEntered = function()
{
    var uninstall_script="function Controller() { gui.clickButton(buttons.NextButton);  gui.clickButton(buttons.NextButton); installer.uninstallationFinished.connect(this, this.uninstallationFinished);} \n" +
                         "Controller.prototype.uninstallationFinished = function() { gui.clickButton(buttons.NextButton); } \n" +
                         "Controller.prototype.FinishedPageCallback = function() { gui.clickButton(buttons.FinishButton); } \n" ;

    var dir = installer.value("TargetDir");
    if (systemInfo.productType === "windows" && installer.fileExists(dir) && installer.fileExists(dir + "/maintenancetool.exe")) {
        if(!installer.fileExists(dir + "/scripts/auto_uninstall.qs")){
            //need to create the uninstall script
            installer.execute("md " + dir + "/scripts");  //make directory if is empty.
            installer.execute("echo ", uninstall_script + " > " + dir + "/scripts/auto_uninstall.qs"); //add auto remove script.
        }
        installer.execute(dir + "/maintenancetool.exe", "--script=" + dir + "/scripts/auto_uninstall.qs");
    }
    else if (systemInfo.productType === "osx" && installer.fileExists(dir) && installer.fileExists(dir + "/maintenancetool.app")) {
        if(!installer.fileExists(dir + "/scripts/auto_uninstall.qs")){
            //need to create the uninstall script
            installer.execute("mkdir " + dir + "/scripts");  //make directory if is empty.
            installer.execute("touch ", uninstall_script + " > " + dir + "/scripts/auto_uninstall.qs"); //add auto remove script.
        }
        installer.execute(dir + "/maintenancetool.app", "--script=" + dir + "/scripts/auto_uninstall.qs");
    }
    else if (systemInfo.productType === "x11" && installer.fileExists(dir) && installer.fileExists(dir + "/maintenancetool")) {
        if(!installer.fileExists(dir + "/scripts/auto_uninstall.qs")){
            //need to create the uninstall script
            installer.execute("mkdir " + dir + "/scripts");  //make directory if is empty.
            installer.execute("touch ", uninstall_script + " > " + dir + "/scripts/auto_uninstall.qs"); //add auto remove script.
        }
        installer.execute(dir + "/maintenancetool", "--script=" + dir + "/scripts/auto_uninstall.qs");
    }

}



//function Controller()
//{
//	 installer.installationFinished.connect(function() {
//
//			 var isUpdate = installer.isUpdater();
//			
//			 if(isUpdate)
//			 {
//				 var targetDir = installer.value("TargetDir");
//				 console.log("targetDir: " + targetDir);
//				 installer.executeDetached(targetDir + "/unseenp2p.app");			 
//				 
//			 }else{
//				var result = QMessageBox.question("quit.question", "Start Program", "Do you want to start the installed application?",QMessageBox.Yes | QMessageBox.No);
//				if( result == QMessageBox.Yes)
//				 {
//					var targetDir = installer.value("TargetDir");
//				  	console.log("targetDir: " + targetDir);
//					console.log("Is Updater: " + installer.isUpdater());
//					console.log("Is Uninstaller: " + installer.isUninstaller());
//					console.log("Is Package Manager: " + installer.isPackageManager());
//					installer.executeDetached(targetDir+"/unseenp2p.app");			 
//				}
//				 
//			 }
//			 
//		
//    });
//	installer.updateFinished.connect(function(){
//		var targetDir = installer.value("TargetDir");
//		console.log("targetDir: " + targetDir);
//		installer.executeDetached(targetDir+"/unseenp2p.app");	
//	});
//
//}
