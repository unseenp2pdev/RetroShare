#include "cert_exchange.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <QtNetwork>
#include  <iostream>

#define DEBUG_CERT_EXCHANGE 1
//unseenp2p - meiyousixin - using text file to save all supernode IPs
static const std::string supernodeIPListFileName = "supernode.list";

int CertExchange::loadAllSupernodeListIPs()
{

    FILE *fd = fopen(supernodeIPListFileName.c_str(), "r");
    if (!fd)
    {
        fprintf(stderr, "Failed to Open File: %s ... No Supernode Nodes\n", supernodeIPListFileName.c_str());
        return 0;
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


#ifdef DEBUG_CERT_EXCHANGE
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
    //QNetworkRequest request(QUrl("http://64.62.233.36:80/rsPeers/acceptInvite"));
    QString urlStr = QString::fromStdString(ip)  + ":" + QString::fromStdString(port) + "/rsPeers/acceptInvite";

    QNetworkRequest request(urlStr);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject json;
    json.insert("invite",cert.c_str());

    //Now you have to create a Network Access object which will help with sending the request.
    QNetworkAccessManager nam;

    //And send (POST) your JSON request. Result will be saved in a QNetworkRply
    QNetworkReply *reply = nam.post(request, QJsonDocument(json).toJson());

    //You have to wait for the response from server.
    while (!reply->isFinished())
    {
        qApp->processEvents();
    }
    QByteArray response_data = reply->readAll();

    printf(" result of json1 is :\n %s", response_data.toStdString().c_str());

    QJsonDocument json_result = QJsonDocument::fromJson(response_data);
    QJsonObject jsonObject = json_result.object();
    QString returnValue = jsonObject["retval"].toString();
    reply->deleteLater();
    if (returnValue == "true")
    {
        return 1;
    }
    else return 0;

}

std::string CertExchange::getSupernodeCert(std::string ip, std::string port)
{
    QString urlStr = QString::fromStdString(ip)  + ":" + QString::fromStdString(port) + "/rsPeers/GetRetroshareInvite";
    QNetworkRequest request(urlStr);
    //QNetworkRequest request(QUrl("http://64.62.233.36:80/rsPeers/GetRetroshareInvite"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject json;
     QNetworkAccessManager nam;
     //And send (POST) your JSON request. Result will be saved in a QNetworkRply

     QNetworkReply *reply = nam.post(request, QJsonDocument(json).toJson());
    //You have to wait for the response from server.

    while (!reply->isFinished())
    {
        qApp->processEvents();
    }
    QByteArray response_data = reply->readAll();

    printf(" \n================\n");
    printf(" result of json2 (supernode certificate) is :\n %s", response_data.toStdString().c_str());

    QJsonDocument json_result = QJsonDocument::fromJson(response_data);
    //One last thing to note is that you have to clean up the reply object by yourself using the line below:

    reply->deleteLater();

    QJsonObject jsonObject = json_result.object();
    QString returnValue = jsonObject["retval"].toString();

    return returnValue.toStdString();

}
