#include "record-analysis.h"
#include "recorder.h"
#include "settings.h"
#include "engine.h"

#include <QFile>
#include <QMessageBox>

RecAnalysis::RecAnalysis(QString dir){
    initialize(dir);
}

void RecAnalysis::initialize(QString dir){
    QList<QString> records_line;
    if(dir.isEmpty())
        records_line = ClientInstance->getRecords();
    else if(dir.endsWith(".png")){
        QByteArray data = Replayer::PNG2TXT(dir);
        QString record_data(data);
        records_line = record_data.split("\n");
    }
    else if(dir.endsWith(".txt")){
        QFile file(dir);
        if(file.open(QIODevice::ReadOnly)){
            QTextStream stream(&file);
            while(!stream.atEnd()){
                QString line = stream.readLine();
                records_line << line;
            }
        }
    }
    else{
        //Weird, I cannot find the header file of tr function
    //    QMessageBox::warning(NULL, tr("Warning"), tr("The file is unreadable"));
    }

    QList<QString> role_list;
    m_recordMap["sgs1"] = new PlayerRecordStruct;
    m_recordMap["sgs1"]->m_screenName = Config.UserName;
    foreach(QString line, records_line){
        if(line.contains("setup")){
            QRegExp rx("(.*):(\\w+):(\\w+):(.*):(\\w+)");
            if(!rx.exactMatch(line))
                continue;

            QStringList texts = rx.capturedTexts();
            QList<QString> ban_packages = texts.at(4).split("+");
            foreach(Package *package, Sanguosha->findChildren<Package *>()){
                if(!ban_packages.contains(package->objectName()) &&
                        Sanguosha->getScenario(package->objectName()) == NULL)
                    m_recordPackages << package->objectName();
            }

            continue;
        }

        if(line.contains("arrangeSeats")){
            role_list = line.split(" ").last().split("+");

            continue;
        }

        if(line.contains("addPlayer")){
            PlayerRecordStruct *player_rec = new PlayerRecordStruct;
            QList<QString> info_assemble = line.split(" ").last().split(":");
            player_rec->m_screenName = QString::fromUtf8(QByteArray::fromBase64(info_assemble.at(1).toAscii()));
            m_recordMap[info_assemble.at(0)] = player_rec;

            continue;
        }

        if(line.contains("general")){
            foreach(QString object, m_recordMap.keys()){
                if(line.contains(object)){
                    QString general = line.split(",").last();
                    general.remove("\"");
                    general.remove("]");

                    line.contains("general2") ?
                            m_recordMap[object]->m_general2Name = general :
                            m_recordMap[object]->m_generalName = general;
                }
            }

            continue;
        }

        if(line.contains("state") && line.contains("robot")){
            foreach(QString object_name, m_recordMap.keys()){
                if(line.contains(object_name)){
                    m_recordMap[object_name]->m_statue = "robot";
                    break;
                }
            }

            continue;
        }

        if(line.contains("hpChange")){
            QList<QString> info_assemble = line.split(" ").last().split(":");
            QString hp = info_assemble.at(1);
            bool ok = false;
            int hp_change = hp.remove(QRegExp("[TF]")).toInt(&ok);
            if(!ok)
                continue;

            if(hp_change > 0)
                m_recordMap[info_assemble.at(0)]->m_recover += hp_change;

            continue;
        }

        if(line.contains("#Damage")){
            QRegExp rx("(.*):(.*)::(\\d+):(\\w+)");
            if(!rx.exactMatch(line))
                continue;

            QStringList texts = rx.capturedTexts();
            QString object_damage = line.contains("#DamageNoSource") ? QString() : texts.at(2).split("->").first();
            QString object_damaged = texts.at(2).split("->").last();
            int damage = texts.at(3).toInt();

            if(!object_damage.isEmpty()) m_recordMap[object_damage]->m_damage += damage;
            m_recordMap[object_damaged]->m_damaged += damage;
            continue;
        }

        if(line.contains("#Murder")){
            QString object = line.split(":").at(1).split("->").first();
            m_recordMap[object]->m_kill++;
            object = line.split(":").at(1).split("->").last();
            m_recordMap[object]->m_isAlive = false;

            continue;
        }
    }

    QString winners = records_line.last();
    winners.remove(winners.lastIndexOf("[")-1, winners.length());
    winners = winners.split("[").last();
    m_recordWinners = winners.remove("\"").split("+");

    winners = records_line.last().remove("\"");
    winners.remove(0, winners.lastIndexOf("[")+1);
    QList<QString> roles_order = winners.remove("]").split(",");
    int i = 0;
    for(; i<role_list.length(); i++)
        m_recordMap[role_list.at(i)]->m_role = roles_order.at(i);
}

PlayerRecordStruct *RecAnalysis::getPlayerRecord(const Player *player) const{
    if(m_recordMap.keys().contains(player->objectName()))
        return m_recordMap[player->objectName()];
    else
        return NULL;
}

QMap<QString, PlayerRecordStruct *> RecAnalysis::getRecordMap() const{
    return m_recordMap;
}

QList<QString> RecAnalysis::getRecordPackages() const{
    return m_recordPackages;
}

QList<QString> RecAnalysis::getRecordWinners() const{
    return m_recordWinners;
}

PlayerRecordStruct::PlayerRecordStruct()
    :m_statue("online"), m_recover(0), m_damage(0),
      m_damaged(0), m_kill(0), m_isAlive(true)
{}
