#include "cert_exchange.h"
//#include <string.h> //for C, for Windows is not good
#include <stdlib.h>
//#include <stdio.h> //for C, for Windows is not good if compiling with C++
#include <QtNetwork>
#include <iostream> //for C++ library
#include <retroshare/rsinit.h>
#include <QObject>

//#define CERT_EXCHANGE_DEBUG 1
//unseenp2p - meiyousixin - using text file to save all supernode IPs
static const std::string supernodeIPListFileName = "supernode.txt";

static bool copyToFile(const std::list<std::string>& data, const std::string &full_path){

    QString filename(full_path.c_str());
    QFile file(filename);
    if (file.open(QIODevice::ReadWrite)) {
            QTextStream stream(&file);
            for(std::string ipaddr : data) {
                stream << ipaddr.c_str() << endl;
            }
            stream.flush();
            file.close();

            return true;
    }else
        return false;

}

int CertExchange::loadAllSupernodeListIPs()
{

    std::string supernodeFile = RsAccounts::AccountDirectory();
    if (supernodeFile != "")
        supernodeFile += "/";
    supernodeFile += supernodeIPListFileName;

    FILE *fd = fopen(supernodeFile.c_str(), "r");
    if (!fd)
    {
#ifdef CERT_EXCHANGE_DEBUG
        fprintf(stderr, "Failed to Open File: %s ... No Supernode Nodes\n", supernodeIPListFileName.c_str());
#endif
        if (!copyToFile(default_seeds,supernodeFile)) {
            return false;
        }
    	fd = fopen(supernodeFile.c_str(), "r");

    }

    supernodeList.clear();
    ip_port.clear();
    char line[10240];
    char addr_str[10240];
    unsigned short port;
    while(line == fgets(line, 10240, fd))
    {
        if (2 == sscanf(line, "%s %hd", addr_str, &port))
        {

                supernodeList.push_back(addr_str);
                ip_port[addr_str] = std::to_string(port);
#ifdef CERT_EXCHANGE_DEBUG
                printf( "Supernode : %s with port: %s \n",addr_str, ip_port[addr_str].c_str());
#endif
        }
    }

    fclose(fd);


    return 1;

}

std::list<std::string> CertExchange::getSupernodeList()
{
    return supernodeList;
}

std::map<std::string,std::string> CertExchange::getIP_Port()
{
    return ip_port;
}

std::map<std::string,std::string> CertExchange::getSupernodeCertList()
{
    return supernodeCertList;
}

int CertExchange::submitCertToSuperNode(std::string ip, std::string port, std::string cert)
{
    QString urlStr = "http://" + QString::fromStdString(ip)  + ":" + QString::fromStdString(port) + "/rsPeers/acceptInvite";

    QNetworkRequest request(urlStr);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject json;
    json.insert("invite",QString::fromStdString(cert));

    //Now you have to create a Network Access object which will help with sending the request.
    QNetworkAccessManager nam;

    //And send (POST) your JSON request. Result will be saved in a QNetworkRply
    QNetworkReply *reply = nam.post(request, QJsonDocument(json).toJson());

    //You have to wait for the response from server.
    QTimer timer;
    timer.setSingleShot(true);

    QEventLoop loop;
    QObject::connect(&timer, SIGNAL(timeout()), &loop, SLOT(quit()));
    QObject::connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));

    timer.start(3000);   // 3 secs. timeout
    loop.exec();
    while (!reply->isFinished())
    {
        qApp->processEvents();

        if(timer.isActive())
        {
            timer.stop();
            if(reply->error() > 0) {
                break;
            }
            else {
              int v = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

              if (v >= 200 && v < 300) {  // Success
                  break;
              }
            }
        }
        else {
           // timeout
           QObject::disconnect(reply, SIGNAL(finished()), &loop, SLOT(quit()));

           reply->abort();
#ifdef CERT_EXCHANGE_DEBUG
           printf("Timeout !!! This server has problem with web service: acceptInvite! ");
#endif
           break;
        }
    }

    if (timer.isActive())
    {
         //printf("Remaining Time = %d \n",  timer.remainingTime() );
         //printf("Time for running this service = %d \n",  timer.interval() - timer.remainingTime() );
         QByteArray response_data = reply->readAll();

         reply->deleteLater();

         if (response_data.size() > 0)
         {
#ifdef CERT_EXCHANGE_DEBUG
             printf(" result of json is :\n %s", response_data.toStdString().c_str());
#endif

             QJsonDocument json_result = QJsonDocument::fromJson(response_data);
             QJsonObject jsonObject = json_result.object();
             bool returnV = jsonObject["retval"].toBool();
             if (returnV)
             {
                 return 1;
             }
             else return 0;

         }
         else return 0;
    }
    else
    {
        reply->deleteLater();
        return 0;
    }
}

std::string CertExchange::getSupernodeCert(std::string ip, std::string port)
{
    //QNetworkRequest request(QUrl("http://64.62.233.36:80/rsPeers/GetRetroshareInvite"));
    QString urlStr = "http://" + QString::fromStdString(ip)  + ":" + QString::fromStdString(port) + "/rsPeers/GetRetroshareInvite";
    QNetworkRequest request(urlStr);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject json;
    QNetworkAccessManager nam;
     //And send (POST) your JSON request. Result will be saved in a QNetworkRply

     QNetworkReply *reply = nam.post(request, QJsonDocument(json).toJson());
    //You have to wait for the response from server.

     //You have to wait for the response from server.
     QTimer timer;
     timer.setSingleShot(true);

     QEventLoop loop;
     QObject::connect(&timer, SIGNAL(timeout()), &loop, SLOT(quit()));
     QObject::connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));

     timer.start(3000);   // 3 secs. timeout
     loop.exec();
     while (!reply->isFinished())
     {
         qApp->processEvents();

         if(timer.isActive())
         {
             timer.stop();
             if(reply->error() > 0) {
                 break;
             }
             else {
               int v = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

               if (v >= 200 && v < 300) {  // Success
                   break;
               }
             }
         }
         else {
            // timeout
            QObject::disconnect(reply, SIGNAL(finished()), &loop, SLOT(quit()));

            reply->abort();
#ifdef CERT_EXCHANGE_DEBUG
            printf("Timeout !!! This server has problem with web service: GetRetroshareInvite! ");
#endif
            break;
         }
     }

     if (timer.isActive())
     {
         QByteArray response_data = reply->readAll();

#ifdef CERT_EXCHANGE_DEBUG
          printf("Remaining Time = %d \n",  timer.remainingTime() );
          printf("Time for running this service = %d \n",  timer.interval() - timer.remainingTime() );
          printf(" \n================\n");
          printf(" result of json2 (supernode certificate) is :\n %s", response_data.toStdString().c_str());
#endif
          QJsonDocument json_result = QJsonDocument::fromJson(response_data);
          reply->deleteLater();

          QJsonObject jsonObject = json_result.object();
          QString returnValue = jsonObject["retval"].toString();

          return returnValue.toStdString();
     }
     else
     {
         reply->deleteLater();
         return "";
     }



}
