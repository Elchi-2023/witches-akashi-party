//////////////////////////////////////////////////////////////////////////////////////
//    akashi - a server for Attorney Online 2                                       //
//    Copyright (C) 2020  scatterflower                                             //
//                                                                                  //
//    This program is free software: you can redistribute it and/or modify          //
//    it under the terms of the GNU Affero General Public License as                //
//    published by the Free Software Foundation, either version 3 of the            //
//    License, or (at your option) any later version.                               //
//                                                                                  //
//    This program is distributed in the hope that it will be useful,               //
//    but WITHOUT ANY WARRANTY; without even the implied warranty of                //
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                 //
//    GNU Affero General Public License for more details.                           //
//                                                                                  //
//    You should have received a copy of the GNU Affero General Public License      //
//    along with this program.  If not, see <https://www.gnu.org/licenses/>.        //
//////////////////////////////////////////////////////////////////////////////////////
#include "aoclient.h"

#include "config_manager.h"
#include "crypto_helper.h"
#include "db_manager.h"
#include "server.h"

// This file is for commands under the authentication category in aoclient.h
// Be sure to register the command in the header before adding it here!

void AOClient::cmdLogin(int argc, QStringList argv){
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    if (m_authenticated || m_vip_authenticated)
        sendServerMessage("You are already logged in!");
    else{
        switch (ConfigManager::authType()) {
        case DataTypes::AuthType::SIMPLE:
            if (ConfigManager::modpass().isEmpty())
                sendServerMessage("No modpass is set. Please set a modpass before logging in.");
            else{
                sendServerMessage("Entering login prompt.\nPlease enter the server modpass.");
                m_is_logging_in = true;
            }
            break;
        case DataTypes::AuthType::ADVANCED:
            sendServerMessage("Entering login prompt.\nPlease enter your username and password.");
            m_is_logging_in = true;
            break;
        }
    }
}

void AOClient::cmdChangeAuth(int argc, QStringList argv){
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    if (ConfigManager::authType() == DataTypes::AuthType::SIMPLE) {
        change_auth_started = true;
        sendServerMessage("WARNING!\nThis command will change how logging in as a moderator works.\nOnly proceed if you know what you are doing\nUse the command /rootpass to set the password for your root account.");
    }
}

void AOClient::cmdSetRootPass(int argc, QStringList argv){
    Q_UNUSED(argc);

    if (!change_auth_started)
        return;

    if (checkPasswordRequirements("root", argv[0])){
        sendServerMessage("Changing auth type and setting root password.\nLogin again with /login root [password]");
        m_authenticated = false;
        ConfigManager::setAuthType(DataTypes::AuthType::ADVANCED);
        server->getDatabaseManager()->createUser("root", CryptoHelper::randbytes(16), argv[0], ACLRolesHandler::SUPER_ID);
    }
    else
        sendServerMessage("Password does not meet server requirements.");
}

void AOClient::cmdAddUser(int argc, QStringList argv){
    Q_UNUSED(argc);

    if (checkPasswordRequirements(argv[0], argv[1]))
        sendServerMessage(server->getDatabaseManager()->createUser(argv[0], CryptoHelper::randbytes(16), argv[1], ACLRolesHandler::NONE_ID) ? "Created user " + argv[0] + ".\nUse /setperms to modify their permissions." : "Unable to create user " + argv[0] + ".\nDoes a user with that name already exist?");
    else
        sendServerMessage("Password does not meet server requirements.");
}

void AOClient::cmdRemoveUser(int argc, QStringList argv){
    Q_UNUSED(argc);
    sendServerMessage(server->getDatabaseManager()->deleteUser(argv[0]) ? "Successfully removed user " + argv[0] + "." : "Unable to remove user " + argv[0] + ".\nDoes it exist?");
}

void AOClient::cmdListPerms(int argc, QStringList argv){
    Q_UNUSED(argc)

    if (argv.isEmpty()){ /* > user role < */
        const ACLRole userrole = server->getACLRolesHandler()->getRoleById(m_acl_role_id);
        if (userrole.getPermissions() == ACLRole::NONE)
            sendServerMessage("\n=== [Permissions] ===\n · NONE");
        else if (userrole.checkPermission(ACLRole::SUPER))
            sendServerMessage("\n=== [Permissions] ===\n ⚠️ SUPER (Be careful! This grants the user all permissions.)");
        else{
            QStringList permslist;

            const QList<ACLRole::Permission> l_permissions = ACLRole::PERMISSION_CAPTIONS.keys();
            for (const ACLRole::Permission i_permission : l_permissions) {
                if (userrole.checkPermission(i_permission)){
                    const QString perms = ACLRole::PERMISSION_CAPTIONS.value(i_permission);
                    if (perms.toLower() == "none")
                        permslist.append(" · NONE");
                    else if (perms.toLower() == "super")
                        permslist.append(" ⚠️ SUPER");
                    else
                        permslist.append(" ⋅ " + perms);
                }
            }
        }
    }
    else if (server->getACLRolesHandler()->getRoleById(m_acl_role_id).checkPermission(ACLRole::MODIFY_USERS)){ /* > target role "if" the caller have perms < */
        const ACLRole userrole = server->getACLRolesHandler()->getRoleById(argv[0]);
        if (userrole.getPermissions() == ACLRole::NONE)
            sendServerMessage("\n=== [" + argv[0] + "\'s Permissions] ===\n · NONE");
        else if (userrole.checkPermission(ACLRole::SUPER))
            sendServerMessage("\n=== [" + argv[0] + "\'s Permissions] ===\n ⚠️ SUPER (Be careful! This grants the user all permissions.)");
        else{
            QStringList permslist;

            const QList<ACLRole::Permission> l_permissions = ACLRole::PERMISSION_CAPTIONS.keys();
            for (const ACLRole::Permission i_permission : l_permissions) {
                if (userrole.checkPermission(i_permission)){
                    const QString perms = ACLRole::PERMISSION_CAPTIONS.value(i_permission);
                    if (perms.toLower() == "none")
                        permslist.append(" · NONE");
                    else if (perms.toLower() == "super")
                        permslist.append(" ⚠️ SUPER");
                    else
                        permslist.append(" ⋅ " + perms);
                }
            }
            sendServerMessage("\n=== [" + argv[0] + "\'s Permissions] ===\n" + permslist.join('\n'));
        }
    }
    else
        sendServerMessage("You do not have permission to view other users' permissions.");
}

void AOClient::cmdSetPerms(int argc, QStringList argv){
    Q_UNUSED(argc);

    const QPair<QString, QString> Target_ACL = {argv[0], argv[1]}; // name and role
    if (server->getACLRolesHandler()->roleExists(Target_ACL.second)){ /* > check if role target are exist < */
        if (Target_ACL.first.toLower().trimmed() != "root"){ /* > exception(s) "root" that can mods allowed < */
            if (Target_ACL.second == ACLRolesHandler::SUPER_ID && !checkPermission(ACLRole::SUPER)){ // if they not have "super" perms, don't let mods set target "super"..
                sendServerMessage("You aren't allowed to set that role!");
                return;
            }
            sendServerMessage(server->getDatabaseManager()->updateACL(Target_ACL.first, Target_ACL.second) ? "Successfully applied role " + Target_ACL.second + " to user " + Target_ACL.first : Target_ACL.first + " wasn't found!");
        }
        else // otherwise.. tell them to don't touch "root" roles
            sendServerMessage("You can't change root's role!");
    }
    else
        sendServerMessage("That role doesn't exist!");
}

void AOClient::cmdRemovePerms(int argc, QStringList argv)
{
    argv.append(ACLRolesHandler::NONE_ID);
    cmdSetPerms(argc, argv);
}

void AOClient::cmdListUsers(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    sendServerMessage("All users:\n" + server->getDatabaseManager()->getUsers().join("\n"));
}

void AOClient::cmdLogout(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    if (m_authenticated){
        sendServerMessage(QString("You are logout from %1, %2.").arg(m_acl_role_id, m_moderator_name));
        m_authenticated = false;
        m_acl_role_id.clear();
        m_moderator_name.clear();
        sendPacket("AUTH", {"-1"}); // Client: "You were logged out."
        Q_EMIT ModeratorObserver();
    }
    else if (m_vip_authenticated){
        sendServerMessage("You are logout from VIP.");
        m_vip_authenticated = false;
        m_acl_role_id.clear();
        m_moderator_name.clear();
    }
    else
        sendServerMessage("You are not logged in!");
}

void AOClient::cmdChangePassword(int argc, QStringList argv){
    QString l_username;
    const QString l_password = argv[0];

    switch (argc){
    case 0: default:
        sendServerMessage("Invalid command syntax.");
        return;
    case 1:
        if (m_authenticated || m_vip_authenticated)
            l_username = m_moderator_name;
        else{
            sendServerMessage("You are not logged in.");
            return;
        }
        break;
    case 2:
        if (checkPermission(ACLRole::SUPER))
            l_username = argv[1];
        else{
            sendServerMessage("Invalid command syntax.");
            return;
        }
        break;
    }

    if (checkPasswordRequirements(l_username, l_password))
        sendServerMessage(server->getDatabaseManager()->updatePassword(l_username, l_password) ? "Successfully changed password." : "There was an error changing the password.");
    else
        sendServerMessage("Password does not meet server requirements.");
}
