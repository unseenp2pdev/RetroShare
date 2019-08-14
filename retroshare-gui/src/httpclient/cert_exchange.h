#include <iostream>
#include <string>
#include <list>
#include <map>
#include <QObject>
#include <QtNetwork>

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
