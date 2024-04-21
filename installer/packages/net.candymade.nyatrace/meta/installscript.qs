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
    // constructor
    component.loaded.connect(this, Component.prototype.loaded);
    if (!installer.addWizardPage(component, "IPDBIntro", QInstaller.InstallationFinished))
        console.log("Could not add the dynamic page.");
    
    if (systemInfo.productType === "windows")
        installer.installationFinished.connect(this, Component.prototype.installVCRedist); 
    
}

Component.prototype.isDefault = function()
{
    // select the component by default
    return true;
}

Component.prototype.createOperations = function()
{
    try {
        // call the base create operations function
        component.createOperations();
        if (installer.value("os") == "win") {
            try {
                var userProfile = installer.environmentVariable("USERPROFILE");
                installer.setValue("UserProfile", userProfile);

                // Create desktop shortcut
                component.addOperation("CreateShortcut", "@TargetDir@\\NyaTrace.exe", "@UserProfile@\\Desktop\\NyaTrace.lnk");

                // Create start menu shortcut
                component.addOperation("CreateShortcut", "@TargetDir@\\NyaTrace.exe", "@StartMenuDir@\\NyaTrace.lnk");
            } catch (e) {
                // Do nothing if key doesn't exist
            }
        }
    } catch (e) {
        console.log(e);
    }
}

Component.prototype.loaded = function ()
{
    var page = gui.pageByObjectName("IPDBIntro");
    if (page != null) {
        console.log("Connecting the dynamic page entered signal.");
    }
}

// Refer to https://stackoverflow.com/a/53342235
Component.prototype.installVCRedist = function()
{
    var registryVC2017x64 = installer.execute("reg", new Array("QUERY", "HKEY_LOCAL_MACHINE\\SOFTWARE\\Wow6432Node\\Microsoft\\VisualStudio\\14.0\\VC\\Runtimes\\x64", "/v", "Installed"))[0];
    var doInstall = false;
    if (!registryVC2017x64) {
        doInstall = true;
    }
    else
    {
        var bld = installer.execute("reg", new Array("QUERY", "HKEY_LOCAL_MACHINE\\SOFTWARE\\Wow6432Node\\Microsoft\\VisualStudio\\14.0\\VC\\Runtimes\\x64", "/v", "Bld"))[0];

        var elements = bld.split(" ");

        bld = parseInt(elements[elements.length-1]);
        if (bld < 26706)
        {
            doInstall = true;
        }
    }

    if (doInstall)
    {
        QMessageBox.information("vcRedist.install", "安装 MSVC++ 基础环境", "在您的系统上我们没有检测到该应用程序的运行需要 MSVC++ 环境。请不要忘了安装它（您通常可以在获得此安装包的附近位置找到它）。", QMessageBox.OK);
        // var dir = installer.value("TargetDir");
        // installer.execute(dir + "/VC_redist.x64.exe", "/norestart", "/passive");
    }
}
