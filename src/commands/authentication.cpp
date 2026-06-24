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

    if (isAuthenticated())
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
        m_authenticated_type = AuthenticateType::NONE;
        ConfigManager::setAuthType(DataTypes::AuthType::ADVANCED);
        server->getDatabaseManager()->CreateUser("root", {CryptoHelper::randbytes(16), argv[0]}, 2);
    }
    else
        sendServerMessage("Password does not meet server requirements.");
}

void AOClient::cmdChangeRootName(int argc, QStringList argv){
    Q_UNUSED(argc)

    if (m_authenticated_type != AuthenticateType::ROOT)
        sendServerMessage("You do not have permission to use that command."); // any user beside [ROOT].. reject.
    else{
        QPointer<DBManager> GetDBManager(server->getDatabaseManager());
        if (GetDBManager->updateUser(m_moderator_name, argv.join("_"))){
            sendServerMessage(QString("You are successfully change the root name from %1 to %2.").arg(m_moderator_name, argv.join("_")));
            m_moderator_name = argv.join("_");
        }
        else
            sendServerMessage("Unsuccessfully change the root name, it does exist or same name?");
    }
}

void AOClient::cmdAddUser(int argc, QStringList argv){
    Q_UNUSED(argc)
    auto GetDBManager = QPointer<DBManager>(server->getDatabaseManager());

    bool u_type_ok;
    const int utype = argv[2].toInt(&u_type_ok);
    if (!u_type_ok || utype < 0 || utype > 1)
        sendServerMessage("Invalid user type.");
    else if (checkPasswordRequirements(argv[0], argv[1]))
        sendServerMessage(GetDBManager->CreateUser(argv[0], {CryptoHelper::randbytes(16), argv[1]}, utype) ? QString("Created user %1 as %2.\nUse /setperms to modify their permissions.").arg(argv[0], QStringList({"VIP", "Moderator"})[utype]) : "Unable to create user " + argv[0] + ".\nDoes a user with that name already exist?");
    else
        sendServerMessage("Password does not meet server requirements.");
}

void AOClient::cmdRemoveUser(int argc, QStringList argv){
    Q_UNUSED(argc);
    sendServerMessage(server->getDatabaseManager()->deleteUser(argv[0]) ? "Successfully removed user " + argv[0] + "." : "Unable to remove user " + argv[0] + ".\nDoes it exist?");
}

void AOClient::cmdListPerms(int argc, QStringList argv){
    Q_UNUSED(argc);

    const QStringList utype({"VIP", "Moderator"});
    QPointer<DBManager> dbManager = server->getDatabaseManager();
    QPointer<ACLRolesHandler> roleHandler = server->getACLRolesHandler();

    // Determine whose permissions to view
    if (argv.isEmpty()) {
        // === View own permissions ===
        const int myType = dbManager->getUserType(m_moderator_name);

        switch (myType) {
        case -1:
            sendServerMessage("You do not have any permissions.");
            break;
        case 2:
            sendServerMessage("You (root) always bypass all permissions.");
            break;
        default: {
            const ACLRole current_role = roleHandler->getRoleById(m_acl_role_id);

            switch (current_role.getPermissions()) {
            case ACLRole::NONE:
                sendServerMessage("You do not have any permissions.");
                break;
            case ACLRole::SUPER:
                sendServerMessage(QString("As %1, you already have all permissions.").arg(utype[myType]));
                break;
            default:
                QStringList permList;
                for (const ACLRole::Permission perm : ACLRole::PERMISSION_CAPTIONS.keys()) {
                    if (perm == ACLRole::NONE)
                        continue;
                    if (current_role.checkPermission(perm))
                        permList.append(QString(" · %1").arg(ACLRole::PERMISSION_CAPTIONS[perm].toUpper().remove("-")));
                }

                sendServerMessage(permList.isEmpty() ? "You do not have any permissions." : QString("\n=== [Your Permissions] ===\n%1").arg(permList.join('\n')));
                break;
            }
            break;
        }
        }
    }
    else if (checkPermission(ACLRole::MODIFY_USERS)) {
        // === View target's permissions ===
        const QString targetName = argv[0];
        const int targetType = dbManager->getUserType(targetName);

        switch (targetType) {
        case -1:
            sendServerMessage("That user does not exist.");
            break;
        case 2:
            sendServerMessage("That user (root) always bypasses all permissions.");
            break;
        default: {
            const QString targetAclId = dbManager->getACL(targetName);
            const ACLRole targetRole = roleHandler->getRoleById(targetAclId);

            switch (targetRole.getPermissions()) {
            case ACLRole::NONE:
                sendServerMessage(QString("%1 does not have any permissions.").arg(targetName));
                break;
            case ACLRole::SUPER:
                sendServerMessage(QString("As %1, %2 already has all permissions.")
                                  .arg(utype[targetType], targetName));
                break;
            default:
                QStringList permList;
                for (const ACLRole::Permission perm : ACLRole::PERMISSION_CAPTIONS.keys()){
                    if (perm == ACLRole::NONE)
                        continue;

                    if (targetRole.checkPermission(perm))
                        permList.append(QString(" · %1").arg(ACLRole::PERMISSION_CAPTIONS[perm].toUpper().remove("-")));
                }
                sendServerMessage(permList.isEmpty() ? QString("%1 does not have any permissions.").arg(targetName) : QString("\n=== [%1's Permissions] ===\n%2").arg(targetName, permList.join('\n')));
                break;
            }
            break;
        }
        }
    }
    else {
        sendServerMessage("You do not have permission to view other users' permissions.");
    }
}

void AOClient::cmdSetPerms(int argc, QStringList argv){
    Q_UNUSED(argc);

    QPointer<ACLRolesHandler> GetRoleHander(server->getACLRolesHandler());
    const QPair<QString, QString> Target_ACL = {argv[0], argv[1]}; // name and role
    if (GetRoleHander->roleExists(Target_ACL.second)){ /* > check if role target are exist < */
        switch (server->getDatabaseManager()->getUserType(Target_ACL.first)){
        case -1:
            sendServerMessage("That username doesn't exist!");
            break;
        default: /* > exception(s) "root" that can mods allowed < */
            if (Target_ACL.second.compare(ACLRolesHandler::SUPER_ID, Qt::CaseInsensitive) == 0) // if they not have "super" perms, don't let mods set target "super"..
                sendServerMessage(m_authenticated_type == AuthenticateType::ROOT ? server->getDatabaseManager()->updateACL(Target_ACL.first, Target_ACL.second) ? "Successfully applied role " + Target_ACL.second + " to user " + Target_ACL.first : Target_ACL.first + " wasn't found!" : "You aren't allowed to set that role!");
            else
                sendServerMessage(server->getDatabaseManager()->updateACL(Target_ACL.first, Target_ACL.second) ? "Successfully applied role " + Target_ACL.second + " to user " + Target_ACL.first : Target_ACL.first + " wasn't found!");
            break;
        case 2: // otherwise.. tell them to don't touch "root" roles
            sendServerMessage("You can't change root's role!");
            break;
        }
    }
    else
        sendServerMessage("That role doesn't exist!");
}

void AOClient::cmdRemovePerms(int argc, QStringList argv)
{
    argv.append(ACLRolesHandler::NONE_ID);
    cmdSetPerms(argc, argv);
}

void AOClient::cmdListUsers(int argc, QStringList argv){
    Q_UNUSED(argc);
    Q_UNUSED(argv);
    QPointer<DBManager> GetDBManager(server->getDatabaseManager());
    const auto current_type = m_version.type;
    QStringList Usertypes;

    for (const QString &user : server->getDatabaseManager()->getUsers()){
        QStringList l_user(user);
        const int l_type = GetDBManager->getUserType(user);
        if (l_type >= 0)
            l_user.prepend(QStringList({"[VIP]", "[M]", current_type == ClientVersion::ClientType::NDS ? "[ROOT]" : "[👑]"})[l_type]);
        if (user == m_moderator_name)
           l_user.prepend(current_type == ClientVersion::ClientType::NDS ? "[YOU]" : " ➤ ");
        Usertypes << l_user.join(' ');
    }

    sendServerMessage("All users:\n" + Usertypes.join("\n"));
}

void AOClient::cmdLogout(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    switch (m_authenticated_type){
    case AOClient::AuthenticateType::NONE:
        sendServerMessage("You are not logged in!");
        break;
    case AOClient::AuthenticateType::VIP:
        sendServerMessage("You are now logout from VIP.");
        m_authenticated_type = AOClient::AuthenticateType::NONE;
        m_acl_role_id.clear();
        m_moderator_name.clear();
        break;
    default:
        sendServerMessage(m_authenticated_type == AOClient::AuthenticateType::MODERATOR ? QString("You are now logout from Moderator, %1.").arg(m_moderator_name) : "You are now logout.");
        m_authenticated_type = AOClient::AuthenticateType::NONE;
        m_acl_role_id.clear();
        m_moderator_name.clear();
        sendPacket("AUTH", {"-1"}); // Client: "You were logged out."
        Q_EMIT ModeratorObserver();
        break;
    }
}

void AOClient::cmdChangePassword(int argc, QStringList argv){
    QString l_username;
    const QString l_password = argv[0];

    switch (argc){
    case 0: default:
        sendServerMessage("Invalid command syntax.");
        return;
    case 1:
        if (isAuthenticated())
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
