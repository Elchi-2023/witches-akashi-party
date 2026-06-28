//////////////////////////////////////////////////////////////////////////////////////
//    akashi - a server for Attorney Online 2                                       //
//    Copyright (C) 2020  scatterflower                                           //
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
#include "db_manager.h"

DBManager::DBManager() :
    DRIVER("QSQLITE")
{
    const QString db_filename = "config/akashi.db";
    QFileInfo db_info(db_filename);
    if (db_info.exists() && (!db_info.isReadable() || !db_info.isWritable())) // We should only check if a file is readable/writeable when it actually exists.
        qCritical() << tr("Database Error: Missing permissions. Check if \"%1\" is writable.").arg(db_filename);
    else if (!db_info.exists())
        qWarning().noquote() << tr("Database Info: Database not found. Attempting to create new database.");

    db = QSqlDatabase::addDatabase(DRIVER);
    db.setDatabaseName(db_filename);
    if (!db.open())
        qCritical() << "[DBManager]: Database Error:" << db.lastError();
    db_version = checkVersion();
    db.exec("CREATE TABLE IF NOT EXISTS bans ('ID' INTEGER, 'IPID' TEXT, 'HDID' TEXT, 'IP' TEXT, 'TIME' INTEGER, 'REASON' TEXT, 'DURATION' INTEGER, 'MODERATOR' TEXT, 'M-TYPE' INTEGER, PRIMARY KEY('ID' AUTOINCREMENT))"); // create ban table if not exist.
    db.exec("CREATE TABLE IF NOT EXISTS users ('ID' INTEGER, 'USERNAME' TEXT, 'SALT' TEXT, 'PASSWORD' TEXT, 'ACL' TEXT, 'TYPE' INTEGER, PRIMARY KEY('ID' AUTOINCREMENT))"); // create users table if not exist.

    /* > indexing < */
    db.exec("CREATE INDEX IF NOT EXISTS idx_bans_ipid ON bans(IPID)");
    db.exec("CREATE INDEX IF NOT EXISTS idx_bans_hdid ON bans(HDID)");
    db.exec("CREATE INDEX IF NOT EXISTS idx_bans_time ON bans(TIME)");
    db.exec("CREATE INDEX IF NOT EXISTS idx_users_username ON users(USERNAME)");

    if (db_version != DB_VERSION)
        updateDB(db_version);

    QSqlQuery users_column = db.exec("SELECT group_concat(name, ',') AS cols FROM pragma_table_info('users')");
    if (users_column.first() && !users_column.value("cols").toString().split(',', Qt::SkipEmptyParts).contains("type", Qt::CaseInsensitive)){ // if "type" (user type) not in "users"...
        db.exec("ALTER TABLE users ADD COLUMN \"TYPE\" INTEGER");
        const QStringList Users = getUsers();
        if (db.transaction()){
            for (const QString& U : Users){ // set type by acl..
                QSqlQuery acl_to_usertype;
                if (U == "root")
                    acl_to_usertype.exec("UPDATE users SET TYPE = 2 WHERE USERNAME = 'root'");
                else{
                    acl_to_usertype.prepare("UPDATE users SET TYPE = ? WHERE USERNAME = ?");
                    acl_to_usertype.addBindValue(getACL(U).toLower() == "vip" ? 0 : 1);
                    acl_to_usertype.addBindValue(U);
                    acl_to_usertype.exec();
                }
            }
            db.commit();
        }
    }
    QSqlQuery bans_column = db.exec("SELECT group_concat(name, ',') AS cols FROM pragma_table_info('bans')");
    if (bans_column.first() && !bans_column.value("cols").toString().split(',', Qt::SkipEmptyParts).contains("m-type", Qt::CaseInsensitive)) // "m-type" (moderator type)..
        db.exec("ALTER TABLE bans ADD COLUMN \"M-TYPE\" INTEGER");
}

QPair<bool, DBManager::BanInfo> DBManager::isIPBanned(const QString &ipid)
{
    QSqlQuery query;
    query.prepare("SELECT * FROM BANS WHERE IPID = ? ORDER BY TIME DESC");
    query.addBindValue(ipid);
    query.exec();
    BanInfo ban;
    if (query.first()) {
        ban.id = query.value(0).toInt();
        ban.ipid = query.value(1).toString();
        ban.hdid = query.value(2).toString();
        ban.ip = QHostAddress(query.value(3).toString());
        ban.time = static_cast<unsigned long>(query.value(4).toULongLong());
        ban.reason = query.value(5).toString();
        ban.duration = query.value(6).toLongLong();
        ban.moderator = query.value(7).toString();
        ban.m_type = query.value(8).isNull() ? -1 : query.value(8).toInt();
        if (ban.duration == -2)
            return {true, ban};
        unsigned long current_time = QDateTime::currentDateTime().toSecsSinceEpoch();
        return {ban.time + ban.duration > current_time, ban};
    }
    else
        return {false, ban};
}

QPair<bool, DBManager::BanInfo> DBManager::isHDIDBanned(const QString &hdid)
{
    QSqlQuery query;
    query.prepare("SELECT * FROM BANS WHERE HDID = ? ORDER BY TIME DESC");
    query.addBindValue(hdid);
    BanInfo ban;
    if (query.exec() && query.first()) {
        ban.id = query.value(0).toInt();
        ban.ipid = query.value(1).toString();
        ban.hdid = query.value(2).toString();
        ban.ip = QHostAddress(query.value(3).toString());
        ban.time = static_cast<unsigned long>(query.value(4).toULongLong());
        ban.reason = query.value(5).toString();
        ban.duration = query.value(6).toLongLong();
        ban.moderator = query.value(7).toString();
        ban.m_type = query.value(8).isNull() ? -1 : query.value(8).toInt();
        if (ban.duration == -2)
            return {true, ban};
        unsigned long current_time = QDateTime::currentDateTime().toSecsSinceEpoch();
        return {ban.time + ban.duration > current_time, ban};
    }
    else
        return {false, ban};
}

int DBManager::getBanID(const QString &hdid)
{
    QSqlQuery query;
    query.prepare("SELECT ID FROM BANS WHERE HDID = ? ORDER BY TIME DESC");
    query.addBindValue(hdid);
    return query.exec() && query.first() ? query.value(0).toInt() : -1;
}

int DBManager::getBanIDByIPID(const QString &ipid)
{
    QSqlQuery query;
    query.prepare("SELECT ID FROM BANS WHERE IPID = ? ORDER BY TIME DESC");
    query.addBindValue(ipid);
    return query.exec() && query.first() ? query.value(0).toInt() : -1;
}

int DBManager::getBanID(QHostAddress ip)
{
    QSqlQuery query;
    query.prepare("SELECT ID FROM BANS WHERE IP = ? ORDER BY TIME DESC");
    query.addBindValue(ip.toString());
    return query.exec() && query.first() ? query.value(0).toInt() : -1;
}

QList<DBManager::BanInfo> DBManager::getRecentBans()
{
    QList<BanInfo> return_list;
    QSqlQuery query;
    query.prepare("SELECT * FROM BANS ORDER BY TIME DESC LIMIT 5");
    query.setForwardOnly(true);
    query.exec();
    while (query.next()) {
        BanInfo ban;
        ban.id = query.value(0).toInt();
        ban.ipid = query.value(1).toString();
        ban.hdid = query.value(2).toString();
        ban.ip = QHostAddress(query.value(3).toString());
        ban.time = static_cast<unsigned long>(query.value(4).toULongLong());
        ban.reason = query.value(5).toString();
        ban.duration = query.value(6).toLongLong();
        ban.moderator = query.value(7).toString();
        ban.m_type = query.value(8).isNull() ? -1 : query.value(8).toInt();
        return_list.append(ban);
    }
    std::reverse(return_list.begin(), return_list.end());
    return return_list;
}

void DBManager::addBan(const BanInfo &ban)
{
    QSqlQuery query;
    query.prepare("INSERT INTO BANS(IPID, HDID, IP, TIME, REASON, DURATION, MODERATOR, \"M-TYPE\") VALUES(?, ?, ?, ?, ?, ?, ?, ?)");
    query.addBindValue(ban.ipid);
    query.addBindValue(ban.hdid);
    query.addBindValue(ban.ip.toString());
    query.addBindValue(QString::number(ban.time));
    query.addBindValue(ban.reason);
    query.addBindValue(ban.duration);
    query.addBindValue(ban.moderator);
    query.addBindValue(ban.m_type);
    if (!query.exec())
        qDebug() << "SQL Error:" << query.lastError().text();
}

bool DBManager::invalidateBan(int id)
{
    QSqlQuery ban_exists;
    ban_exists.prepare("SELECT DURATION FROM bans WHERE ID = ?");
    ban_exists.addBindValue(id);
    ban_exists.exec();

    if (!ban_exists.first())
        return false;

    QSqlQuery query;
    query.prepare("UPDATE bans SET DURATION = 0 WHERE ID = ?");
    query.addBindValue(id);
    return query.exec();
}

bool DBManager::CreateUser(const QString &username, const QPair<QByteArray, QString> &password, const int u_type){
    QSqlQuery username_exists;
    username_exists.prepare("SELECT ACL FROM users WHERE USERNAME = ?");
    username_exists.addBindValue(username);
    username_exists.exec();

    if (username_exists.first())
        return false;

    QSqlQuery query;

    QString salted_password = CryptoHelper::hash_password(password.first, password.second);

    query.prepare("INSERT INTO users(USERNAME, SALT, PASSWORD, ACL, TYPE) VALUES(?, ?, ?, ?, ?)");
    query.addBindValue(username);
    query.addBindValue(password.first.toHex());
    query.addBindValue(salted_password);
    query.addBindValue(u_type == 2 ? "ROOT" : "NONE");
    query.addBindValue(u_type);
    return query.exec();
}

bool DBManager::deleteUser(const QString &username){
    if (getUserType(username) == 2)
        return false; // To prevent lockout scenarios where an admin may accidentally delete root.
    else{
        QSqlQuery username_exists;
        username_exists.prepare("SELECT EXISTS(SELECT USERNAME FROM users WHERE USERNAME = ?)");
        username_exists.addBindValue(username);
        username_exists.exec();
        username_exists.first();
        if (username_exists.value(0).toInt() == 0) // If EXISTS can't find a record, it returns 0.
            return false; // We were unable to locate an entry with this name.

        QSqlQuery username_delete;
        username_delete.prepare("DELETE FROM users WHERE USERNAME = ?");
        username_delete.addBindValue(username);
        return username_delete.exec();
    }
}

QString DBManager::getACL(const QString &f_username)
{
    if (f_username.isEmpty())
        return {};

    QSqlQuery query;
    query.prepare("SELECT ACL FROM users WHERE USERNAME = ?");
    query.addBindValue(f_username);
    return query.exec() && query.first() ? query.value(0).toString() : QString();
}

int DBManager::getUserType(const QString &f_username){
    if (f_username.isEmpty())
        return -1;

    QSqlQuery query;
    query.prepare("SELECT TYPE FROM users WHERE USERNAME = ?");
    query.addBindValue(f_username);
    return query.exec() && query.first() ? query.value(0).toInt() : -1;
}
bool DBManager::authenticate(const QString& username, const QString& password){
    QSqlQuery query;
    query.prepare("SELECT SALT, PASSWORD FROM users WHERE USERNAME = ?");
    query.addBindValue(username);

    if (!query.exec() || !query.first())
        return false;

    const QString salt = query.value(0).toString();
    const QString stored_pass = query.value(1).toString();
    const QByteArray saltBytes = QByteArray::fromHex(salt.toUtf8());

    const QString salted_password = CryptoHelper::hash_password(saltBytes,password);

    const bool authenticated = (salted_password == stored_pass);

    // Update old-style hashes to new ones on the fly
    if (authenticated && saltBytes.length() < CryptoHelper::pbkdf2_salt_len){
        updatePassword(username, password);
    }

    return authenticated;
}

bool DBManager::updateACL(const QString &f_username, const QString &f_acl)
{
    QSqlQuery l_username_exists;
    l_username_exists.prepare("SELECT ACL FROM users WHERE USERNAME = ?");
    l_username_exists.addBindValue(f_username);

    if (!l_username_exists.exec() || !l_username_exists.first())
        return false;

    QSqlQuery l_update_acl;
    l_update_acl.prepare("UPDATE users SET ACL = ? WHERE USERNAME = ?");
    l_update_acl.addBindValue(f_acl);
    l_update_acl.addBindValue(f_username);
    return l_update_acl.exec();
}

bool DBManager::updateUser(const QString &username, const QString &change){
    QSqlQuery l_username_exists;
    l_username_exists.prepare("SELECT ACL FROM users WHERE USERNAME = ?");
    l_username_exists.addBindValue(username);

    if (!l_username_exists.exec() || !l_username_exists.first() || username == change)
        return false;

    QSqlQuery l_update_acl;
    l_update_acl.prepare("UPDATE users SET USERNAME = ? WHERE USERNAME = ?");
    l_update_acl.addBindValue(change);
    l_update_acl.addBindValue(username);
    return l_update_acl.exec();
}

QStringList DBManager::getUsers()
{
    QStringList users;

    QSqlQuery query(db);
    query.prepare("SELECT USERNAME FROM users ORDER BY ID");
    query.setForwardOnly(true);
    if (query.exec()){
        while (query.next())
            users.append(query.value(0).toString());
    }

    return users;
}

QList<DBManager::BanInfo> DBManager::getBanInfo(const QString &lookup_type, const QString &id)
{
    const QHash<QString, QString> match_type{{"banid", "SELECT * FROM BANS WHERE ID = ?"}, {"hdid", "SELECT * FROM BANS WHERE HDID = ?"}, {"ipid", "SELECT * FROM BANS WHERE IPID = ?"}};
    QSqlQuery query;

    if (match_type.contains(lookup_type))
        query.prepare(match_type[lookup_type]);
    else{
        qCritical("[DBManager]: Invalid ban lookup type!");
        return {};
    }
    query.addBindValue(id);
    query.setForwardOnly(true);
    QList<BanInfo> return_list;
    if (query.exec()){
        while (query.next()) {
            BanInfo ban;
            ban.id = query.value(0).toInt();
            ban.ipid = query.value(1).toString();
            ban.hdid = query.value(2).toString();
            ban.ip = QHostAddress(query.value(3).toString());
            ban.time = static_cast<unsigned long>(query.value(4).toULongLong());
            ban.reason = query.value(5).toString();
            ban.duration = query.value(6).toLongLong();
            ban.moderator = query.value(7).toString();
            ban.m_type = query.value(8).isNull() ? -1 : query.value(8).toInt();
            return_list.append(ban);
        }
    }
    std::reverse(return_list.begin(), return_list.end());
    return return_list;
}

bool DBManager::updateBan(int ban_id, const QString &field, const QVariant &updated_info){
    QSqlQuery query;
    if (field == "reason") {
        query.prepare("UPDATE bans SET REASON = ? WHERE ID = ?");
        query.addBindValue(updated_info.toString());
    }
    else if (field == "duration") {
        query.prepare("UPDATE bans SET DURATION = ? WHERE ID = ?");
        query.addBindValue(updated_info.toLongLong());
    }
    else
        return false;

    query.addBindValue(ban_id);

    const bool exec_ok = query.exec();
    if (!exec_ok)
        qDebug() << "[DBManager]: Error while doing update ban" << query.lastError();
    return exec_ok;
}

bool DBManager::updatePassword(const QString &username, const QString &password)
{
    QByteArray salt = CryptoHelper::randbytes(16);
    QString salted_password = CryptoHelper::hash_password(salt, password);

    QSqlQuery query;
    query.prepare("UPDATE users SET PASSWORD = ?, SALT = ? WHERE USERNAME = ?");
    query.addBindValue(salted_password);
    query.addBindValue(salt.toHex());
    query.addBindValue(username);
    return query.exec();
}

int DBManager::checkVersion()
{
    QSqlQuery query;
    query.prepare("PRAGMA user_version");
    return query.exec() && query.first() ? query.value(0).toInt() : 0;
}

void DBManager::updateDB(int current_version)
{
    switch (current_version) {
    case 0:
        QSqlQuery("ALTER TABLE bans ADD COLUMN MODERATOR TEXT");
        Q_FALLTHROUGH();
    case 1:
        QSqlQuery("PRAGMA user_version = " + QString::number(1));
        Q_FALLTHROUGH();
    case 2:
        QSqlQuery("UPDATE users SET ACL = 'SUPER' WHERE TYPE = '2'");
        QSqlQuery("PRAGMA user_version = " + QString::number(DB_VERSION));
        break;
    }
}

DBManager::~DBManager()
{
    db.close();
}
