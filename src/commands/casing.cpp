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

#include "area_data.h"
#include "config_manager.h"
#include "packet/packet_factory.h"
#include "server.h"

// This file is for commands under the casing category in aoclient.h
// Be sure to register the command in the header before adding it here!

void AOClient::cmdDoc(int argc, QStringList argv){
    AreaData *l_area = server->getAreaById(areaId());
    if (argc == 0 || argv.join(" ").trimmed().isEmpty()) // this.. between argc are 0 or the "space" qstring join are empty
        sendServerMessage(l_area->document().isEmpty() ? "No document." : "Document: " + l_area->document());
    else{
        l_area->changeDoc(argv.join(" "));
        sendServerMessageArea(QString("[%1] %2 changed the document.").arg(QString::number(clientId()), character().isEmpty() ? name() : character()));
    }
}

void AOClient::cmdClearDoc(int argc, QStringList argv){
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    AreaData *l_area = server->getAreaById(areaId());
    if (l_area->document().isEmpty())
        sendServerMessage("No document to be clear.");
    else{
        l_area->changeDoc(QString());
        sendServerMessageArea(QString("[%1] %2 cleared the document.").arg(QString::number(clientId()), character().isEmpty() ? name() : character()));
    }
}

void AOClient::cmdEvidenceMod(int argc, QStringList argv){
    Q_UNUSED(argc);

    AreaData *l_area = server->getAreaById(areaId());
    const QMap<QString, AreaData::EvidenceMod> targetEvMod = {{"cm", AreaData::EvidenceMod::CM}, {"mod", AreaData::EvidenceMod::MOD}, {"hiddencm", AreaData::EvidenceMod::HIDDEN_CM}, {"hidden_cm", AreaData::EvidenceMod::HIDDEN_CM}, {"ffa", AreaData::EvidenceMod::FFA}};
    if (targetEvMod.contains(argv[0].toLower())){
        l_area->setEviMod(targetEvMod[argv[0].toLower()]);
        sendServerMessage("Changed evidence mod.");

        // Resend evidence lists to everyone in the area
        sendEvidenceList(l_area);
    }
    else
        sendServerMessage("Invalid evidence mod.");
}

void AOClient::cmdEvidence_Swap(int argc, QStringList argv){
    Q_UNUSED(argc);

    AreaData *l_area = server->getAreaById(areaId());
    const auto l_ev = l_area->evidence();

    if (l_ev.isEmpty())
        sendServerMessage("No evidence in area.");
    else{
        QPair<bool, bool> VaildID = {false, false};
        const QPair<int, int> EvidenceID = {argv[0].toInt(&VaildID.first), argv[1].toInt(&VaildID.second)};
        if (VaildID.first){ /* > first target vaild < */
            if (EvidenceID.first >= 0 && EvidenceID.first < l_ev.size()){ /* first target ID/Index are in ranges */
                if (VaildID.second){ /* second target vaiid to be swaps */
                    if (EvidenceID.second >= 0 && EvidenceID.second < l_ev.size() && EvidenceID.first != EvidenceID.second){ /* > same but make sure it doesn't same ID/Index < */
                        l_area->swapEvidence(EvidenceID.first, EvidenceID.second); /* > swapping process < */
                        sendEvidenceList(l_area);
                        sendServerMessage(QString("The evidence ID [%1] and [%2] have been swapped.").arg(QString("%1: %2").arg(QString::number(EvidenceID.first), l_ev[EvidenceID.first].name.isEmpty() ? "Unamed Evidence" : l_ev[EvidenceID.first].name), QString("%1: %2").arg(QString::number(EvidenceID.second), l_ev[EvidenceID.second].name.isEmpty() ? "Unamed Evidence" : l_ev[EvidenceID.second].name)));
                    }
                    else /* otherwise.. */
                        sendServerMessage("Unable to swap  evidence. second evidence ID out of range/negative/same.");
                }
                else
                    sendServerMessage("Invalid second evidence ID.");
            }
            else
                sendServerMessage("Unable to swap evidence. first evidence ID out of range/negative.");
        }
        else /* otherwise.. invaild */
            sendServerMessage("Invalid evidence ID.");
    }
}

void AOClient::cmdTestify(int argc, QStringList argv){
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    AreaData *l_area = server->getAreaById(areaId());
    if (l_area->testimonyRecording() == AreaData::TestimonyRecording::RECORDING) {
        sendServerMessage("Testimony recording is already in progress. Please stop it with /pause before starting a new one.");
    }
    else {
        clearTestimony();
        l_area->setTestimonyRecording(AreaData::TestimonyRecording::RECORDING);
        sendServerMessage("Started testimony recording. The next IC message will be a title. Use /pause to stop recording.");
    }
}

void AOClient::cmdExamine(int argc, QStringList argv){
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    AreaData *l_area = server->getAreaById(areaId());

    switch (l_area->testimony().size()){
    default: /* cause.. the qvector:first() is title so skipped */
        l_area->restartTestimony();
        sendServerPacketArea(PacketFactory::createPacket("RT", {"testimony2", "0"}));
        sendServerPacketArea(PacketFactory::createPacket("MS", {l_area->testimony()[0]}));
        break;
    case 1: case 0: /* it doesn't matter if just 1 or empty anyways.. (since that's how works on og akashi) */
        sendServerMessage("Unable to start replay without prior testimony. Use /testify to start or load a testimony with the command: /loadtestimony.");
        break;
    }
}

void AOClient::cmdTestimony(int argc, QStringList argv){
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    AreaData *l_area = server->getAreaById(areaId());
    switch (l_area->testimony().size()){
    case 0: case 1:
        sendServerMessage("Unable to display empty testimony.");
        break;
    default:
        {
            QStringList testimonyindex;
            const int current_statement = l_area->statement();
            for (int i = 1; i <= l_area->testimony().size() - 1; i++){
                if (current_statement >= 1 && i == current_statement)
                    testimonyindex.append(QString(" ➤ [%1] %2").arg(QString::number(i), QStringList(l_area->testimony()[i])[4]));
                else
                    testimonyindex.append(QString(" · [%1] %2").arg(QString::number(i), QStringList(l_area->testimony()[i])[4]));
            }
            sendServerMessage(QString("\n=== 📰 %1 📰 ===\n%2\n === testimoy ===").arg(l_area->testimony().first().value(4).remove(0, 2), testimonyindex.join('\n')));
        }
        break;
    }
}

void AOClient::cmdDeleteStatement(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    AreaData *l_area = server->getAreaById(areaId());
    int l_c_statement = l_area->statement();
    if (l_area->testimony().size() - 1 == 0)
        sendServerMessage("Unable to delete statement. No statements saved in this area.");
    else if (l_c_statement > 0 && l_area->testimony().size() > 2) {
        l_area->removeStatement(l_c_statement);
        sendServerMessage("The statement with id " + QString::number(l_c_statement) + " has been deleted from the testimony.");
    }
}

void AOClient::cmdUpdateStatement(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    server->getAreaById(areaId())->setTestimonyRecording(AreaData::TestimonyRecording::UPDATE);
    sendServerMessage("The next IC-Message will replace the currently selected testimony line.");
}

void AOClient::cmdPauseTestimony(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    AreaData *l_area = server->getAreaById(areaId());
    l_area->setTestimonyRecording(AreaData::TestimonyRecording::STOPPED);
    sendServerPacketArea(PacketFactory::createPacket("RT", {"testimony1", "1"}));
    sendServerMessage("Testimony has been stopped. Use /examine to begin cross-examination.");
}

void AOClient::cmdAddStatement(int argc, QStringList argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    if (server->getAreaById(areaId())->statement() < ConfigManager::maxStatements()) {
        server->getAreaById(areaId())->setTestimonyRecording(AreaData::TestimonyRecording::ADD);
        sendServerMessage("The next IC-Message will be inserted into the testimony.");
    }
    else
        sendServerMessage("Unable to add anymore statements. Please remove any unused ones.");
}

void AOClient::cmdSaveTestimony(int argc, QStringList argv){
    Q_UNUSED(argc);

    if (checkPermission(ACLRole::SAVETEST) || m_testimony_saving){
        AreaData *l_area = server->getAreaById(areaId());
        if (l_area->testimony().size() - 1 <= 0)
            sendServerMessage("Can't save an empty testimony.");
        else{
            QDir l_dir_testimony("storage/testimony");
            if (!l_dir_testimony.exists())
                l_dir_testimony.mkpath(".");

            const QString l_testimony_name = argv[0].trimmed().toLower().replace("..", ""); // :)
            QFile l_file("storage/testimony/" + l_testimony_name + ".txt");
            if (l_file.exists())
                sendServerMessage("Unable to save testimony. Testimony name already exists.");
            else{
                QTextStream l_out(&l_file);
                if (l_file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)){
                    for (int i = 0; i <= l_area->testimony().size() - 1; i++)
                        l_out << l_area->testimony().at(i).join("#") << "\n";
                    sendServerMessage("Testimony saved. To load it use /loadtestimony " + l_testimony_name);
                    m_testimony_saving = false;
                }
                else
                    sendServerMessage("Unable to save testimony. Permission denied.");
            }
        }
    }
    else
        sendServerMessage("You don't have permission to save a testimony. Please contact a moderator for permission.");
}

void AOClient::cmdLoadTestimony(int argc, QStringList argv)
{
    Q_UNUSED(argc);

    AreaData *l_area = server->getAreaById(areaId());
    QDir l_dir_testimony("storage/testimony");
    if (l_dir_testimony.exists()){
        QString l_testimony_name = argv[0].trimmed().toLower().replace("..", ""); // :)
        QFile l_file("storage/testimony/" + l_testimony_name + ".txt");

        if (l_file.exists()){
            if (l_file.open(QIODevice::ReadOnly | QIODevice::Text)){
                clearTestimony();
                int l_testimony_lines = 0;
                QTextStream l_in(&l_file);
                while (!l_in.atEnd()) {
                    if (l_testimony_lines <= ConfigManager::maxStatements()) {
                        QString line = l_in.readLine();
                        QStringList packet = line.split("#");
                        l_area->addStatement(l_area->testimony().size(), packet);
                        l_testimony_lines = l_testimony_lines + 1;
                    }
                    else {
                        sendServerMessage("Testimony too large to be loaded.");
                        clearTestimony();
                        return;
                    }
                }
                sendServerMessage("Testimony loaded successfully. Use /examine to start playback.");
            }
            else
                sendServerMessage("Unable to load testimony. Permission denied.");
        }
        else
            sendServerMessage("Unable to load testimony. Testimony name not found.");
    }
    else
        sendServerMessage("Unable to load testimonies. Testimony storage not found.");
}
