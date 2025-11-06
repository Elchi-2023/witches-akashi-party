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
#include "discord.h"

#include "config_manager.h"

Discord::Discord(QObject *parent) :
    QObject(parent)
{
    m_nam = new QNetworkAccessManager();
    connect(m_nam, &QNetworkAccessManager::finished,
            this, &Discord::onReplyFinished);
}

void Discord::onModcallWebhookRequested(const QStringList &f_name, const QString &f_area, const QString &f_reason, const QQueue<QString> &f_buffer)
{
    m_request.setUrl(QUrl(ConfigManager::discordModcallWebhookUrl()));
    QJsonDocument l_json = constructModcallJson(f_name, f_area, f_reason);
    postJsonWebhook(l_json);

    if (ConfigManager::discordModcallWebhookSendFile()) {
        QHttpMultiPart *l_multipart = constructLogMultipart(f_buffer);
        postMultipartWebhook(*l_multipart);
    }
}

void Discord::onBanWebhookRequested(const QString &f_ipid, const QString &f_moderator, const QString &f_duration, const QString &f_reason, const int &f_banID, const int &f_count)
{
    m_request.setUrl(QUrl(ConfigManager::discordBanWebhookUrl()));
    QJsonDocument l_json = constructBanJson(f_ipid, f_moderator, f_duration, f_reason, f_banID, f_count);
    postJsonWebhook(l_json);
}

QJsonDocument Discord::constructModcallJson(const QStringList &f_name, const QString &f_area, const QString &f_reason) const
{
    QJsonArray fields;

    // Field 1: Who the caller
    QJsonObject field1;
    field1["name"] = "Who the caller";
    field1["value"] = f_name[0];
    field1["inline"] = true;
    fields.append(field1);

    // Field 2: On Area
    QJsonObject field2;
    field2["name"] = "In Area";
    field2["value"] = f_area;
    field2["inline"] = true;
    fields.append(field2);

    // Field 3: Reason
    QJsonObject field3;
    field3["name"] = "Reason";
    field3["value"] = f_reason;
    fields.append(field3);

    if (f_name.size() >= 2){ /* Field 4: Regarding */
        QJsonObject field4;
        field4["name"] = "Regarding";
        field4["value"] = f_name[1] == f_name[0] ? "[the caller itself]" : f_name[1];
        fields.append(field4);
    }

    // Create the embed object
    QJsonObject embed;
    embed["title"] = "Modcall Report";
    embed["fields"] = fields;
    embed["color"] = ConfigManager::discordWebhookColor();

    // Create the embeds array
    QJsonArray embeds;
    embeds.append(embed);

    // Create the root object
    QJsonObject root;
    root["embeds"] = embeds;
    if (!ConfigManager::discordModcallWebhookContent().isEmpty())
        root["content"] = ConfigManager::discordModcallWebhookContent();

    return QJsonDocument(root);
}

QJsonDocument Discord::constructBanJson(const QString &f_ipid, const QString &f_moderator, const QString &f_duration, const QString &f_reason, const int &f_banID, const int &f_counts)
{
    const QJsonObject discordBanData = {
        {"embeds", QJsonArray{
            QJsonObject{
                {"title", "Ban Report"},
                {"color", ConfigManager::discordWebhookColor()},
                {"fields", QJsonArray{
                    QJsonObject{
                        {"name", "IPID"},
                        {"value", f_ipid},
                        {"inline", true}
                    },
                    QJsonObject{
                        {"name", "ID"},
                        {"value", QString::number(f_banID)},
                        {"inline", true}
                    },
                    QJsonObject{
                        {"name", "Issued by"},
                        {"value", f_moderator},
                        {"inline", true}
                    },
                    QJsonObject{
                        {"name", "Until"},
                        {"value", f_duration},
                        {"inline", true}
                    },
                    QJsonObject{
                        {"name", "Clients Count"},
                        {"value", f_counts},
                        {"inline", true}
                    },
                    QJsonObject{
                        {"name", "Reason"},
                        {"value", f_reason}
                    }
                }}
            }
        }}
    };

    return QJsonDocument(discordBanData);
}

QHttpMultiPart *Discord::constructLogMultipart(const QQueue<QString> &f_buffer) const
{
    QHttpMultiPart *l_multipart = new QHttpMultiPart();
    QHttpPart l_logdata;
    l_logdata.setHeader(QNetworkRequest::ContentDispositionHeader, "form-data; name=\"file\"; filename=\"log.txt\"");
    l_logdata.setHeader(QNetworkRequest::ContentTypeHeader, "text/plain; charset=utf-8");
    QString l_log;
    for (const QString &log_entry : f_buffer) {
        l_log.append(log_entry);
    }
    l_logdata.setBody(l_log.toUtf8());
    l_multipart->append(l_logdata);
    return l_multipart;
}

void Discord::postJsonWebhook(const QJsonDocument &f_json)
{
    if (!QUrl(m_request.url()).isValid()) {
        qWarning("Invalid webhook URL!");
        return;
    }
    m_request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    m_nam->post(m_request, f_json.toJson());
}

void Discord::postMultipartWebhook(QHttpMultiPart &f_multipart)
{
    if (!QUrl(m_request.url()).isValid()) {
        qWarning("Invalid webhook URL!");
        f_multipart.deleteLater();
        return;
    }
    m_request.setHeader(QNetworkRequest::ContentTypeHeader, "multipart/form-data; boundary=" + f_multipart.boundary());
    QNetworkReply *l_reply = m_nam->post(m_request, &f_multipart);
    f_multipart.setParent(l_reply);
}

void Discord::onReplyFinished(QNetworkReply *f_reply)
{
    auto l_data = f_reply->readAll();
    f_reply->deleteLater();
#ifdef DISCORD_DEBUG
    QDebug() << l_data;
#else
    Q_UNUSED(l_data);
#endif
}

Discord::~Discord()
{
    m_nam->deleteLater();
}
