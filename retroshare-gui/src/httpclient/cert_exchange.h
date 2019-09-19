#ifndef CERTEXCHANGE_H
#define CERTEXCHANGE_H

#include <iostream>
#include <string>
#include <list>
#include <map>
#include <QObject>
#include <QtNetwork>
#include <QFile>
#include <QCoreApplication>
#include <QTextStream>

static const std::list<std::string> default_seeds ({
    	"64.62.233.36 80",
    	"64.62.233.37 80",
	"64.62.233.53 80",
	"82.221.107.66 80",
	"82.221.107.74 80",
	"82.221.107.81 80"
});


class CertExchange
{
public:
    CertExchange() {}
    int loadAllSupernodeListIPs();
    std::list<std::string> getSupernodeList();
    std::map<std::string,std::string> getIP_Port();
    std::map<std::string,std::string> getSupernodeCertList();
    int submitCertToSuperNode(std::string ip, std::string port, std::string cert);
    std::string getSupernodeCert(std::string ip, std::string port);
private:

    std::list<std::string> supernodeList;
    std::map<std::string,std::string>  ip_port;
    std::map<std::string,std::string>  supernodeCertList;
};



#endif // CERTEXCHANGE_H
