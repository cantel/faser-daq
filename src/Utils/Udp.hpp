/*
  Copyright (C) 2019-2020 CERN for the benefit of the FASER collaboration
*/
#ifndef UDP_H_
#define UDP_H_

#include <string>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>

class UdpSender {
public:
  UdpSender() : m_socket(0), m_addr(0) {};
  ~UdpSender() {
    freeaddrinfo(m_addr);
    if (m_socket) close(m_socket);
  };
  int init(const std::string& host,int port) {
    struct addrinfo hints;
    struct addrinfo *addrResult;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;    /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_DGRAM; /* Datagram socket */
    hints.ai_flags = 0;    /* For wildcard IP address */
    hints.ai_protocol = 0;          /* Any protocol */
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

    int s = getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hints, &addrResult);
    if (s != 0) return s;
    // Creating socket file descriptor
    int sockfd=-1;
    if ( (sockfd = socket(addrResult->ai_family, addrResult->ai_socktype,
			  addrResult->ai_protocol)) < 0 ) { 
	freeaddrinfo(addrResult);
	return -1;
    }
    m_socket=sockfd;
    m_addr=addrResult;
    return 0;
  };
  int send(void *data,int len) {
    return sendto(m_socket,data,len,MSG_CONFIRM,m_addr->ai_addr,m_addr->ai_addrlen);
  };
private:
  int m_socket;
  addrinfo *m_addr;
};
  
class UdpReceiver {
public:
  UdpReceiver() : m_socket(0) {};
  ~UdpReceiver() {
    if (m_socket) close(m_socket);
  };
  int init(int port) {
    int sockFD;
    if ( (sockFD = socket(AF_INET,SOCK_DGRAM,0)) < 0) {
      return -1;
    }
    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr)); 
    servaddr.sin_family    = AF_INET; // IPv4 
    servaddr.sin_addr.s_addr = INADDR_ANY; 
    servaddr.sin_port = htons(port);
  
    if (bind(sockFD,(const struct sockaddr *)&servaddr,  
	     sizeof(servaddr))==-1) {
      return -1;
    }
    struct timeval tv;
    tv.tv_sec = 1; // one second time out on recv - hardcoded for now
    tv.tv_usec = 0;
    setsockopt(sockFD, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);

    m_socket=sockFD;
    return 0;
  }
  int receive(void *data,int maxLen) {
     return recv(m_socket, data, maxLen,MSG_WAITALL);
  }
  
private:
  int m_socket;
  };
#endif /* UDP_H_ */
