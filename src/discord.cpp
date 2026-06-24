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

#include <QStringList>

Discord::Discord(QObject *parent) :
    QObject(parent)
{
    m_nam = new QNetworkAccessManager();
    connect(m_nam, &QNetworkAccessManager::finished,
            this, &Discord::onReplyFinished);
}

void Discord::onModcallWebhookRequested(const QPair<QString, QString> &f_name, const QPair<QString, QString> &r_name, const QString &f_reason, const QQueue<QString> &f_buffer)
{
    m_request.setUrl(QUrl(ConfigManager::discordModcallWebhookUrl()));
    QJsonDocument l_json = constructModcallJson(f_name, r_name, f_reason);
    postJsonWebhook(l_json);

    if (ConfigManager::discordModcallWebhookSendFile() && !f_buffer.isEmpty()) {
        QHttpMultiPart *l_multipart = constructLogMultipart(f_buffer);
        postMultipartWebhook(*l_multipart);
    }
}

void Discord::onBanWebhookRequested(const QString &f_ipid, const QPair<int, QString> &f_moderator, const QString &f_duration, const QString &f_reason, const int &f_banID, const int &f_count)
{
    m_request.setUrl(QUrl(ConfigManager::discordBanWebhookUrl()));
    QJsonDocument l_json = constructBanJson(f_ipid, f_moderator, f_duration, f_reason, f_banID, f_count);
    postJsonWebhook(l_json);
}
void Discord::onUnbanWebhookRequested(const QString &f_ipid, const QPair<QPair<int, QString>, QPair<int, QString> > &f_moderator, const int &f_banID, const int &f_ban_duration, const QDateTime &f_date, const QStringList &f_reason){
    m_request.setUrl(QUrl(ConfigManager::discordBanWebhookUrl()));
    postJsonWebhook(constructUnbanJson(f_ipid, f_moderator, f_banID, f_ban_duration, f_date, f_reason));
}

QJsonDocument Discord::constructModcallJson(const QPair<QString, QString> &f_name, const QPair<QString, QString> &r_name, const QString &f_reason) const
{
    QJsonArray fields;

    // Field 1: Who the caller
    QJsonObject field1;
    field1["name"] = "Who the caller";
    field1["value"] = f_name.first;
    field1["inline"] = true;
    fields.append(field1);

    // Field 2: On Area
    QJsonObject field2;
    field2["name"] = "In Area";
    field2["value"] = f_name.second;
    field2["inline"] = true;
    fields.append(field2);

    if (!r_name.first.isEmpty()){ /* Field 3: Regarding */
        QJsonObject field3;
        field3["name"] = "Regarding";
        field3["value"] = r_name.first == f_name.first ? "[the caller itself]" : f_name.first;
        field3["inline"] = true;
        fields.append(field3);
        if (!r_name.second.isEmpty()){
            QJsonObject field3_ex;
            field3_ex["name"] = "In Area (Regarding)";
            field3_ex["value"] = r_name.second;
            field3_ex["inline"] = true;
            fields.append(field3_ex);
        }
    }
    // Field 4: Reason
    QJsonObject field4;
    field4["name"] = "Reason";
    field4["value"] = f_reason;
    fields.append(field4);

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

QJsonDocument Discord::constructBanJson(const QString &f_ipid, const QPair<int, QString> &f_moderator, const QString &f_duration, const QString &f_reason, const int &f_banID, const int &f_counts){
    QJsonArray fields;

    fields.append(QJsonObject{
                      {"name", "IPID"},
                      {"value", f_ipid},
                      {"inline", true}
                  });

    fields.append(QJsonObject{
                      {"name", "ID"},
                      {"value", QString::number(f_banID)},
                      {"inline", true}
                  });

    fields.append(QJsonObject{
                      {"name", "Issued by"},
                      {"value", f_moderator.second},
                      {"inline", true}
                  });

    if (f_moderator.first >= 0)
        fields.append(QJsonObject{
                          {"name", "Moderator Type"},
                          {"value", QStringList({"Mini (VIP)", "Normal", "[ROOT]"})[f_moderator.first]},
                          {"inline", true}
                      });

    fields.append(QJsonObject{
                      {"name", "Until"},
                      {"value", f_duration},
                      {"inline", true}
                  });

    if (f_counts >= 1) /* if target is online */
        fields.append(QJsonObject{
                          {"name", "Matched Client/IPIDs Count"},
                          {"value", f_counts == 1 ? "Single" : QString::number(f_counts)},
                          {"inline", true}
                      });

    fields.append(QJsonObject{
                      {"name", "Reason"},
                      {"value", f_reason}
                  });

    const QJsonObject Root = {
        {"embeds", QJsonArray{
             QJsonObject{
                 {"title", "Ban Report"},
                 {"color", ConfigManager::discordWebhookColor()},
                 {"fields", fields}
             }}
        }
    };

    return QJsonDocument(Root);
}

QJsonDocument Discord::constructUnbanJson(const QString &f_ipid, const QPair<QPair<int, QString>, QPair<int, QString> > &f_moderator, const int &f_banID, const int &f_ban_duration, const QDateTime &f_date, const QStringList &f_reason){
    // --- fields array ---
    QJsonArray fields;

    fields.append(QJsonObject{
                      {"name", "IPID"},
                      {"value", f_ipid},
                      {"inline", true}
                  });

    fields.append(QJsonObject{
                      {"name", "ID"},
                      {"value", QString::number(f_banID)},
                      {"inline", true}
                  });

    fields.append(QJsonObject{
                      {"name", "Banned by"},
                      {"value", f_moderator.first.second},
                      {"inline", true}
                  });
    if (f_moderator.first.first >= 0)
        fields.append(QJsonObject{
                          {"name", "[Banned by] Type"},
                          {"value", QStringList({"Mini (VIP)", "Normal", "[ROOT]"})[f_moderator.first.first]},
                          {"inline", true}
                      });
    fields.append(QJsonObject{
                      {"name", "Revoked by"},
                      {"value", f_moderator.second.second},
                      {"inline", true}
                  });
    if (f_moderator.second.first >= 0)
        fields.append(QJsonObject{
                          {"name", "[Revoked by] Type"},
                          {"value", QStringList({"Mini (VIP)", "Normal", "[ROOT]"})[f_moderator.second.first]},
                          {"inline", true}
                      });

    fields.append(QJsonObject{
                      {"name", "Ban Date"},
                      {"value", f_ban_duration >= 0 ? QString("<t:%1:R>").arg(f_date.addSecs(f_ban_duration).toSecsSinceEpoch()) : "Undefined / Permanently"},
                      {"inline", true}
                  });
    fields.append(QJsonObject{
                      {"name", "Revoked Date"},
                      {"value", QString("<t:%1:R>").arg(QDateTime::currentDateTime().toSecsSinceEpoch())},
                      {"inline", true}
                  });

    if (!f_reason.isEmpty()){
        fields.append(QJsonObject{{"name", "The Ban Reason"},{"value", f_reason[0].isEmpty() ? "No Reason Provided" : f_reason[0]}, {"inline", true}});
        if (f_reason.size() > 1)
            fields.append(QJsonObject{{"name", "Reason of Revoked"},{"value", f_reason[1].isEmpty() ? "No Reason Provided" : f_reason[1]}});
    }

    // --- embed object ---
    QJsonObject embed{
        {"title", "Unban Report"},
        {"fields", fields},
        {"color", ConfigManager::discordWebhookColor()}
    };

    // --- root ---
    QJsonObject root;
    root["embeds"] = QJsonArray{ embed };
    root["content"] = "@here";

    return QJsonDocument(root);
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

void Discord::onReplyFinished(QNetworkReply *f_reply){
    const QScopedPointer<QNetworkReply, QScopedPointerDeleteLater> reply(f_reply);
#ifdef DISCORD_DEBUG
    QDebug() << reply->readAll();
#endif
}

Discord::~Discord()
{
    m_nam->deleteLater();
}
