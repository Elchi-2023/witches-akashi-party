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
#include "logger/u_logger.h"

ULogger::ULogger(QObject *parent) :
    QObject(parent)
{
    switch (ConfigManager::loggingType()) {
    case DataTypes::LogType::MODCALL:
        writerModcall = new WriterModcall;
        break;
    case DataTypes::LogType::FULL:
    case DataTypes::LogType::FULLAREA:
        writerFull = new WriterFull;
        break;
    }
    loadLogtext();
}

ULogger::~ULogger()
{
    switch (ConfigManager::loggingType()) {
    case DataTypes::LogType::MODCALL:
        writerModcall->deleteLater();
        break;
    case DataTypes::LogType::FULL:
    case DataTypes::LogType::FULLAREA:
        writerFull->deleteLater();
        break;
    }
}

void ULogger::logIC(const QString &f_char_name, const QString &f_ooc_name, const QPair<int, QString> &f_ids, const QString &f_area_name, const QString &f_message){
    const QString l_time = QDateTime::currentDateTime().toString("ddd MMMM d yyyy | hh:mm:ss");
    const QString l_logEntry = QString(m_logtext.value("ic")).replace("<time>", l_time).replace("<area>", f_area_name)
            .replace("<id>", QString::number(f_ids.first)).replace("<char>", f_char_name).replace("<ooc>", f_ooc_name)
            .replace("<ipid>", f_ids.second).replace("<message>", f_message);
    updateAreaBuffer(f_area_name, l_logEntry + '\n');
}

void ULogger::logOOC(const QString &f_char_name, const QString &f_ooc_name, const QPair<int, QString> &f_ids, const QString &f_area_name, const QString &f_message){
    const QString l_time = QDateTime::currentDateTime().toString("ddd MMMM d yyyy | hh:mm:ss");
    const QString l_logEntry = QString(m_logtext.value("occ")).replace("<time>", l_time).replace("<area>", f_area_name)
            .replace("<id>", QString::number(f_ids.first)).replace("<char>", f_char_name).replace("<ooc>", f_ooc_name)
            .replace("<ipid>", f_ids.second).replace("<message>", f_message);
    updateAreaBuffer(f_area_name, l_logEntry + "\n");
}

void ULogger::logMusic(const QString &f_char_Name, const QString &f_ooc_name, const QPair<int, QString> &f_ids, const QString &f_area_name, const QString &f_track){
    const QString l_time = QDateTime::currentDateTime().toString("ddd MMMM d yyyy | hh:mm:ss");
    const QString l_logEntry = QString(m_logtext.value("occ")).replace("<time>", l_time).replace("<area>", f_area_name)
            .replace("<id>", QString::number(f_ids.first)).replace("<char>", f_char_Name).replace("<ooc>", f_ooc_name)
            .replace("<ipid>", f_ids.second).replace("<track>", f_track);
    updateAreaBuffer(f_area_name, l_logEntry + "\n");
}
void ULogger::logLogin(const QString &f_char_name, const QString &f_ooc_name, const QString &f_moderator_name,const QString &f_ipid, const QString &f_area_name, const bool &f_success){
    const QString l_time = QDateTime::currentDateTime().toString("ddd MMMM d yyyy | hh:mm:ss");
    const QString l_logEntry = QString(m_logtext.value("login")).replace("<time>", l_time).replace("<area>", f_area_name)
            .replace("<char>", f_char_name).replace("<ooc>", f_ooc_name)
            .replace("<ipid>", f_ipid).replace("<passed>", QStringList({"FALLED", "SUCCESS, Logined as " + f_moderator_name})[f_success]);
    updateAreaBuffer(f_area_name, l_logEntry + '\n');
}

void ULogger::logCMD(const QString &f_char_name, const QString &f_ipid, const QString &f_ooc_name, const QString &f_command, const QStringList &f_args, const QString &f_area_name){
    const QString l_time = QDateTime::currentDateTime().toString("ddd MMMM d yyyy | hh:mm:ss");
    QString l_logEntry;
    /* Some commands contain sensitive data, like passwords, These must be filtered out */
    const QHash<QString, QString> selected_command{
        {"login", QString(m_logtext.value("cmdlogin")).replace("<time>", l_time).replace("<area>", f_area_name)
                    .replace("<char>", f_char_name).replace("<ooc>", f_ooc_name)
                    .replace("<ipid>", f_ipid)},
        {"rootpass", QString(m_logtext.value("cmdrootpass")).replace("<time>", l_time).replace("<area>", f_area_name)
                    .replace("<char>", f_char_name).replace("<ooc>", f_ooc_name)
                    .replace("<ipid>", f_ipid)},
        {"adduser", QString(m_logtext.value("cmdadduser")).replace("<time>", l_time).replace("<area>", f_area_name)
                    .replace("<char>", f_char_name).replace("<ooc>", f_ooc_name)
                    .replace("<ipid>", f_ipid).replace("<user>", f_args.at(0))}
    };

    if (selected_command.contains(f_command))
        l_logEntry = selected_command[f_command];
    else /* > any cmd < */
        l_logEntry = QString(m_logtext.value("cmd")).replace("<time>", l_time).replace("<area>", f_area_name)
                .replace("<char>", f_char_name).replace("<ooc>", f_ooc_name)
                .replace("<ipid>", f_ipid).replace("<cmd_name>", f_command).replace("<cmd_arg>", f_args.join(' '));
    updateAreaBuffer(f_area_name, l_logEntry + '\n');
}

void ULogger::logKick(const QString &f_moderator, const QString &f_target_ipid){
    const QString l_time = QDateTime::currentDateTime().toString("ddd MMMM d yyyy | hh:mm:ss");
    const QString l_logEntry = QString(m_logtext.value("kick")).replace("<time>", l_time).replace("<m_name>", f_moderator).replace("<ipid>", f_target_ipid);
    updateAreaBuffer("SERVER", l_logEntry + '\n');
}

void ULogger::logBan(const QString &f_moderator, const QString &f_target_ipid, const QString &f_duration){
    const QString l_time = QDateTime::currentDateTime().toString("ddd MMMM d yyyy | hh:mm:ss");
    const QString l_logEntry = QString(m_logtext.value("ban")).replace("<time>", l_time).replace("<m_name>", f_moderator).replace("<ipid>", f_target_ipid).replace("<duration>", f_duration);
    updateAreaBuffer("SERVER", l_logEntry + '\n');
}

void ULogger::logModcall(const QString &f_char_name, const QPair<int, QString> &f_ids, const QString &f_ooc_name, const QString &f_area_name){
    const QString l_time = QDateTime::currentDateTime().toString("ddd MMMM d yyyy | hh:mm:ss");
    const QString l_logEvent = QString(m_logtext.value("modcall")).replace("<time>", l_time).replace("<area>", f_area_name)
            .replace("<id>", QString::number(f_ids.first)).replace("<char>", f_char_name).replace("<ooc>", f_ooc_name)
            .replace("<ipid>", f_ids.second);
    updateAreaBuffer(f_area_name, l_logEvent + '\n');

    if (ConfigManager::loggingType() == DataTypes::LogType::MODCALL)
        writerModcall->flush(f_area_name, buffer(f_area_name));
}

void ULogger::logConnectionAttempt(const QString &f_ip_address, const QString &f_ipid, const QString &f_hwid){
    const QString l_time = QDateTime::currentDateTime().toString("ddd MMMM d yyyy | hh:mm:ss");
    const QString l_logEntry = QString(m_logtext.value("connect")).replace("<time>", l_time).replace("<ip>", f_ip_address).replace("<ipid>", f_ipid).replace("<hwid>", f_hwid);
    updateAreaBuffer("SERVER", l_logEntry + '\n');
}

void ULogger::loadLogtext()
{
    // All of this to prevent one single clazy warning from appearing.
    for (auto iterator = m_logtext.keyBegin(), end = m_logtext.keyEnd(); iterator != end; ++iterator) {
        QString l_tempstring = ConfigManager::LogText(iterator.operator*());
        if (!l_tempstring.isEmpty()) {
            m_logtext[iterator.operator*()] = l_tempstring;
        }
    }
}

void ULogger::updateAreaBuffer(const QString &f_area_name, const QString &f_log_entry)
{
    QQueue<QString> l_buffer = m_bufferMap.value(f_area_name);

    if (l_buffer.length() <= ConfigManager::logBuffer())
        l_buffer.enqueue(f_log_entry);
    else {
        l_buffer.dequeue();
        l_buffer.enqueue(f_log_entry);
    }
    m_bufferMap.insert(f_area_name, l_buffer);

    switch (ConfigManager::loggingType()){
    case DataTypes::LogType::FULL:
        writerFull->flush(f_log_entry, f_area_name);
        break;
    case DataTypes::LogType::FULLAREA:
        writerFull->flush(f_log_entry, f_area_name);
        break;
    default:
        break; /* not having feature implemented yet.. */
    }
}

QQueue<QString> ULogger::buffer(const QString &f_area_name)
{
    return m_bufferMap.value(f_area_name);
}
