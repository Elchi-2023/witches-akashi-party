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
#include "serverpublisher.h"
#include "config_manager.h"
#include "qnamespace.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>
#include <QPointer>

const int WS_REVERSE_PROXY = 80;
const int TIMEOUT = 1000 * 60 * 1;

ServerPublisher::ServerPublisher(int port, int *player_count, QObject *parent) :
    QObject(parent),
    m_manager{new QNetworkAccessManager(this)},
    timeout_timer(new QTimer(this)),
    m_players(player_count),
    m_port{port}
{
    connect(m_manager, &QNetworkAccessManager::finished, this, &ServerPublisher::finished);
    connect(timeout_timer, &QTimer::timeout, this, &ServerPublisher::publishServer);

    timeout_timer->setTimerType(Qt::PreciseTimer);
    timeout_timer->setInterval(TIMEOUT);
    timeout_timer->start();
    publishServer();
}

void ServerPublisher::publishServer(){
    if (ConfigManager::publishServerEnabled()){
        QUrl serverlist(ConfigManager::serverlistURL());
        if (serverlist.isValid()){
            QNetworkRequest request(serverlist);
            request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
#if QT_VERSION_MAJOR < 6 // using this compiler <check-if> instead..
            request.setAttribute(QNetworkRequest::Attribute::Http2AllowedAttribute, false);
#endif

            QJsonObject serverinfo;
            if (!ConfigManager::serverDomainName().trimmed().isEmpty())
                serverinfo["ip"] = ConfigManager::serverDomainName();
            if (ConfigManager::securePort() > -1)
                serverinfo["wss_port"] = ConfigManager::securePort();

            serverinfo["port"] = 27106;
            serverinfo["ws_port"] = ConfigManager::advertiseWSProxy() ? WS_REVERSE_PROXY : m_port;
            serverinfo["players"] = *m_players;
            serverinfo["name"] = ConfigManager::serverName();
            serverinfo["description"] = ConfigManager::serverDescription();

            m_manager->post(request, QJsonDocument(serverinfo).toJson());
        }
        else
            qWarning() << "[W][AKASHI][SERVER-PUBLISHER]: Failed to advertise server. Serverlist URL is not valid. URL:" << serverlist.toString();
    }
}

void ServerPublisher::finished(QNetworkReply *f_reply){
    const QScopedPointer<QNetworkReply, QScopedPointerDeleteLater> reply(f_reply);
    if (reply.isNull()) // safely first..
        qWarning() << "[W][AKASHI][PUBLISHER]: The qnetworkreply object is null, cannot progress the advertises (otherwise segfaults).";
    else{
        switch (reply->error()){
        default: // [ERROR] types..
            qWarning() << "[W][AKASHI][PUBLISHER]:Unable to connect to serverlist due to the following error:" << reply->errorString();
            qWarning() << "[W][AKASHI][PUBLISHER]:Remote URL:" << reply->url().toString();
            break;
        case QNetworkReply::NetworkError::NoError:
            switch (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()){ // status code..
            case 200: // [HTTP_OK]..
                qInfo() << "[I][AKASHI][SERVER-PUBLISHER]: Sucessfully advertised server to serverlist.";
                break;
            default:
                QJsonParseError error;
                const QByteArray Data = reply->readAll();
                const QJsonDocument document = QJsonDocument::fromJson(Data, &error);

                switch (error.error){
                case QJsonParseError::ParseError::NoError:
                    if (document.isObject()){
                        const QJsonObject body = document.object();
                        if (body.contains("errors")){
                            QStringList error_records;
                            for (const auto &ref : body["errors"].toArray()){
                                if (ref.isObject()){
                                    const QJsonObject error_obj = ref.toObject();
                                    error_records << QString("[%1]: %2").arg(error_obj["type"].toString(), error_obj["message"].toString());
                                }
                            }

                            error_records.isEmpty() ? qWarning() << "[W][AKASHI][SERVER-PUBLISHER]: Failed to advertise to the serverlist due to the unknowns errors." : qWarning().noquote() << "[W][AKASHI][SERVER-PUBLISHER]: Failed to advertise to the serverlist due to the following errors:\n" << error_records.join('\n');
                        }
                        else
                            qWarning() << "[W][AKASHI][SERVER-PUBLISHER]: Sucessfully(?) advertised server to serverlist.";
                    }
                    else
                        qWarning().noquote() << QString("[W][AKASHI][SERVER-PUBLISHER]: Received malformed response from MS ([%1] %2 of offset(%3)): %4").arg(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toString(), error.errorString(), QString::number(error.offset), Data);
                    break;
                default:
                    qWarning().noquote() << QString("[W][AKASHI][SERVER-PUBLISHER]: Received malformed response from MS ([%1] %2 of offset(%3)): %4").arg(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toString(), error.errorString(), QString::number(error.offset), Data);
                    break;
                }
            }
            break;
        }
    }
}
