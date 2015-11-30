#include "ofxTCPManager.h"
#include <stdio.h>
#include "ofxNetworkUtils.h"

//--------------------------------------------------------------------------------
bool ofxTCPManager::m_bWinsockInit= false;

//--------------------------------------------------------------------------------
ofxTCPManager::ofxTCPManager()
{
  // was winsock initialized?
  	#ifdef TARGET_WIN32
		if (!m_bWinsockInit) {
			unsigned short vr;
			WSADATA	wsaData;
			vr = MAKEWORD(2,	2);
			WSAStartup(vr, &wsaData);
			m_bWinsockInit=	true;
		}
	#else
		//this disables the other apps from shutting down if the client
		//or server disconnects.
		signal(SIGPIPE,SIG_IGN);
		signal(EPIPE,SIG_IGN);
	#endif

  nonBlocking = false;
  m_hSocket= INVALID_SOCKET;
  m_dwTimeoutSend= OF_TCP_DEFAULT_TIMEOUT;
  m_dwTimeoutReceive= OF_TCP_DEFAULT_TIMEOUT;
  m_dwTimeoutAccept= OF_TCP_DEFAULT_TIMEOUT;
  m_dwTimeoutConnect= 5;//OF_TCP_DEFAULT_TIMEOUT;
  m_iListenPort= -1;
  m_closing = false;
  m_iMaxConnections = 100;
};

//--------------------------------------------------------------------------------
/// Closes an open socket.
/// NOTE: A closed socket cannot be reused again without a call to "Create()".
bool ofxTCPManager::Close()
{
    if (m_hSocket == INVALID_SOCKET) return(true);

	#ifdef TARGET_WIN32
		if(closesocket(m_hSocket) == SOCKET_ERROR)
	#else
		m_closing = true;
		shutdown(m_hSocket,SHUT_RDWR);
		if(close(m_hSocket) == SOCKET_ERROR)
	#endif
		{
			//	if it's reported we're not/no longer a socket, let it fall through and be invalidated
			int Error = ofxNetworkCheckError();
			if ( Error != OFXNETWORK_ERROR(NOTSOCK) )
			{
				return(false);
			}
		}

	m_hSocket= INVALID_SOCKET;

	#ifdef TARGET_WIN32
		//This was commented out in the original
		//WSACleanup();
	#endif
	return(true);
}

//--------------------------------------------------------------------------------
void ofxTCPManager::CleanUp() {
	#ifdef TARGET_WIN32
		WSACleanup();
	#endif
  m_bWinsockInit = false;
}

//--------------------------------------------------------------------------------
bool ofxTCPManager::CheckIsConnected(){
	fd_set fd;
	FD_ZERO(&fd);
	FD_SET(m_hSocket, &fd);
	timeval tv = { (time_t)0, 1 };
	if (select(0, &fd, NULL, NULL, &tv) == 1) {
		int so_error;
		socklen_t len = sizeof so_error;
		getsockopt(m_hSocket, SOL_SOCKET, SO_ERROR, (char*)&so_error, &len);
		if (so_error == 0) {
			u_long toread;
#ifdef TARGET_WIN32
			ioctlsocket(m_hSocket, FIONREAD, &toread);
#else
			ioctl(m_hSocket, FIONREAD, &toread);
#endif
			if (toread == 0) {
				return false;
			}
		}
	}
	return true;
}

//--------------------------------------------------------------------------------
bool ofxTCPManager::Create()
{
	if (m_hSocket != INVALID_SOCKET) return(false);
	m_closing = false;

	m_hSocket = socket( AF_INET, SOCK_STREAM, IPPROTO_IP);

	bool ret = (m_hSocket != INVALID_SOCKET);

	if(!ret) ofxNetworkCheckError();

	return ret;
}


//--------------------------------------------------------------------------------
bool ofxTCPManager::Listen(int iMaxConnections)
{
	if (m_hSocket == INVALID_SOCKET) return(false);
	m_iMaxConnections = iMaxConnections;
	bool ret = (listen(m_hSocket, iMaxConnections)!= SOCKET_ERROR);
	if(!ret) ofxNetworkCheckError();
	return ret;
}

bool ofxTCPManager::Bind(unsigned short usPort)
{
	struct sockaddr_in local;
	memset(&local, 0, sizeof(sockaddr_in));

	local.sin_family = AF_INET;
	local.sin_addr.s_addr = INADDR_ANY;
	//Port MUST be in Network Byte Order
	local.sin_port = htons(usPort);

	if (::bind(m_hSocket,(struct sockaddr*)&local,sizeof(local))){
		ofxNetworkCheckError();
		return false;
	}
	return true;
}

//--------------------------------------------------------------------------------
bool ofxTCPManager::Accept(ofxTCPManager& sConnect)
{
  sockaddr_in addr;

  #ifndef TARGET_WIN32
	socklen_t iSize;
  #else
	int iSize;
  #endif

  if (m_hSocket == INVALID_SOCKET) return(false);

  if (m_dwTimeoutAccept != NO_TIMEOUT) {
	  fd_set fd;
	  FD_ZERO(&fd);
	  FD_SET(m_hSocket, &fd);
	  timeval tv= {(time_t)m_dwTimeoutAccept, 0};
	  if(select(0, &fd, NULL, NULL, &tv) == 0) {
		  ofxNetworkCheckError();
		  return(false);
	  }
  }

  iSize= sizeof(sockaddr_in);
  sConnect.m_hSocket= accept(m_hSocket, (sockaddr*)&addr, &iSize);
  bool ret = (sConnect.m_hSocket != INVALID_SOCKET);
  if(!ret && !m_closing) ofxNetworkCheckError();
  return ret;
}


//--------------------------------------------------------------------------------
bool ofxTCPManager::Connect(char *pAddrStr, unsigned short usPort)
{
  sockaddr_in addr_in= {0};
  struct hostent *he;

  if (m_hSocket == INVALID_SOCKET){
	  return(false);
  }

  if ((he = gethostbyname(pAddrStr)) == NULL)
    return(false);

	addr_in.sin_family= AF_INET; // host byte order
	addr_in.sin_port  = htons(usPort); // short, network byte order
	addr_in.sin_addr  = *((struct in_addr *)he->h_addr);

	// set to non-blocking before connect
	bool wasBlocking = nonBlocking;
	SetNonBlocking(true);

    int ret = connect(m_hSocket, (sockaddr *)&addr_in, sizeof(sockaddr));
    
    // set a timeout
    if (ret < 0 && ofxNetworkCheckError() == OFXNETWORK_ERROR(INPROGRESS) && m_dwTimeoutConnect != NO_TIMEOUT) {
        fd_set fd;
        FD_ZERO(&fd);
        FD_SET(m_hSocket, &fd);
        timeval	tv=	{(time_t)m_dwTimeoutConnect, 0};
        ret = select(m_hSocket+1,NULL,&fd,NULL,&tv);
        if(ret<0){
            ofxNetworkCheckError();
        }
        if(ret== 1) {
            int so_error;
            socklen_t len = sizeof so_error;
            getsockopt(m_hSocket, SOL_SOCKET, SO_ERROR, (char*)&so_error, &len);
            if (so_error == 0) {
                ret = true;
            } 
        }
    }

	SetNonBlocking(wasBlocking);
    
	return ret>=0;
}

//--------------------------------------------------------------------------------
///Theo added - Choose to set nonBLocking - default mode is to block
bool ofxTCPManager::SetNonBlocking(bool useNonBlocking)
{
	if(useNonBlocking==nonBlocking){
		return true;
	}
    auto prevNonBlocking = nonBlocking;
    nonBlocking = useNonBlocking;

	#ifdef TARGET_WIN32
		unsigned long arg = nonBlocking;
		int retVal = ioctlsocket(m_hSocket,FIONBIO,&arg);
	#else
		int flags = fcntl(m_hSocket, F_GETFL, 0);
		int retVal;
		if(useNonBlocking){
			retVal = fcntl(m_hSocket, F_SETFL, flags | O_NONBLOCK);
		}else{
			retVal = fcntl(m_hSocket, F_SETFL, flags & ~O_NONBLOCK);
		}
	#endif

	bool ret = (retVal >= 0);
	if(!ret){
		ofxNetworkCheckError();
		nonBlocking = prevNonBlocking;
	}

	return ret;
}

bool ofxTCPManager::IsNonBlocking(){
    return nonBlocking;
}

//--------------------------------------------------------------------------------
int ofxTCPManager::Write(const char* pBuff, const int iSize)
{
	int iBytesSent= 0;
	int iBytesTemp;
	const char* pTemp= pBuff;

  do {
	  iBytesTemp= Send(pTemp, iSize - iBytesSent);
    // error occured?
    if (iBytesTemp == SOCKET_ERROR) return(SOCKET_ERROR);
    if (iBytesTemp == SOCKET_TIMEOUT) return(SOCKET_TIMEOUT);

		iBytesSent+= iBytesTemp;
		pTemp+= iBytesTemp;
	} while(iBytesSent < iSize);

  return(iBytesSent);
}

//--------------------------------------------------------------------------------
//Theo added - alternative to GetTickCount for windows
//This version returns the milliseconds since the unix epoch
//Should be good enough for what it is being used for here
//(mainly time comparision)
#ifndef TARGET_WIN32
unsigned long GetTickCount(){
  timeb bsdTime;
  ftime(&bsdTime);

   unsigned long msSinceUnix = (bsdTime.time*1000) + bsdTime.millitm;
   return msSinceUnix;
}
#endif

//--------------------------------------------------------------------------------
/// Return values:
/// SOCKET_TIMEOUT indicates timeout
/// SOCKET_ERROR in case of a problem.
int ofxTCPManager::Send(const char* pBuff, const int iSize)
{
    if (m_hSocket == INVALID_SOCKET) return(SOCKET_ERROR);

    if (m_dwTimeoutSend	!= NO_TIMEOUT){
		fd_set fd;
		FD_ZERO(&fd);
		FD_SET(m_hSocket, &fd);
		timeval	tv=	{(time_t)m_dwTimeoutSend, 0};
		if(select(m_hSocket+1,NULL,&fd,NULL,&tv)== 0)
		{
			return(SOCKET_TIMEOUT);
		}
	}
	return send(m_hSocket, pBuff, iSize, 0);
}

//--------------------------------------------------------------------------------
/// Return values:
/// SOCKET_TIMEOUT indicates timeout
/// SOCKET_ERROR in case of a problem.
int ofxTCPManager::SendAll(const char* pBuff, const int iSize)
{
  if (m_hSocket == INVALID_SOCKET) return(SOCKET_ERROR);


	unsigned long timestamp= GetTickCount();

	if (m_dwTimeoutSend	!= NO_TIMEOUT)
	{
		fd_set fd;
		FD_ZERO(&fd);
		FD_SET(m_hSocket, &fd);
		timeval	tv=	{(time_t)m_dwTimeoutSend, 0};
		if(select(m_hSocket+1,NULL,&fd,NULL,&tv)== 0)
		{
			return(SOCKET_TIMEOUT);
		}
	}

	int total= 0;
	int bytesleft = iSize;
	int ret=-1;

	while (total < iSize) {
		ret = send(m_hSocket, pBuff + total, bytesleft, 0);
        if (ret == SOCKET_ERROR) { return SOCKET_ERROR; }
		total += ret;
		bytesleft -=ret;
        if (GetTickCount() - timestamp > m_dwTimeoutSend * 1000) return SOCKET_TIMEOUT;
	}

	return total;
}


//--------------------------------------------------------------------------------
/// Return values:
/// SOCKET_TIMEOUT indicates timeout
/// SOCKET_ERROR in case of a problem.
///
int ofxTCPManager::Receive(char* pBuff, const int iSize)
{
    if (m_hSocket == INVALID_SOCKET) return(SOCKET_ERROR);

    if (m_dwTimeoutReceive	!= NO_TIMEOUT){
  		fd_set fd;
  		FD_ZERO(&fd);
  		FD_SET(m_hSocket, &fd);
  		timeval	tv=	{(time_t)m_dwTimeoutSend, 0};
  		if(select(m_hSocket+1,&fd,NULL,NULL,&tv)== 0)
  		{
  			return(SOCKET_TIMEOUT);
  		}
  	}
	return recv(m_hSocket, pBuff, iSize, 0);
}



//--------------------------------------------------------------------------------
/// Return values:
/// SOCKET_TIMEOUT indicates timeout
/// SOCKET_ERROR in case of a problem.
///
int ofxTCPManager::PeekReceive(char* pBuff, const int iSize)
{
	if (m_hSocket == INVALID_SOCKET) 
		return(SOCKET_ERROR);
 
	return recv(m_hSocket, pBuff, iSize, MSG_PEEK);
}

//--------------------------------------------------------------------------------
/// Return values:
/// SOCKET_TIMEOUT indicates timeout
/// SOCKET_ERROR in case of a problem.
int ofxTCPManager::ReceiveAll(char* pBuff, const int iSize)
{
	if (m_hSocket == INVALID_SOCKET) return(SOCKET_ERROR);

	unsigned long timestamp= GetTickCount();

	if (m_dwTimeoutReceive	!= NO_TIMEOUT)
	{
		fd_set fd;
		FD_ZERO(&fd);
		FD_SET(m_hSocket, &fd);
		timeval	tv=	{(time_t)m_dwTimeoutSend, 0};
		if(select(m_hSocket+1,&fd,NULL,NULL,&tv)== 0)
		{
			return(SOCKET_TIMEOUT);
		}
	}
	int totalBytes=0;

	unsigned long stamp = GetTickCount();

	do {
		int ret= recv(m_hSocket, pBuff+totalBytes, iSize-totalBytes, 0);
		if (ret==0 && totalBytes != iSize){
			if(m_dwTimeoutReceive != NO_TIMEOUT){
				return SOCKET_ERROR;
			}else{
				return SOCKET_TIMEOUT;
			}
		}
		if (ret < 0){
			return SOCKET_ERROR;
		}
		if (GetTickCount() - timestamp > m_dwTimeoutReceive * 1000) return SOCKET_TIMEOUT;
		totalBytes += ret;
		#ifndef TARGET_WIN32
			usleep(2000); //should be 20ms
		#else
			Sleep(2);
		#endif
		if (GetTickCount() - stamp > 10000)
			return SOCKET_TIMEOUT;
	}while(totalBytes < iSize);

	return totalBytes;
}

//--------------------------------------------------------------------------------
bool ofxTCPManager::GetRemoteAddr(LPINETADDR pInetAddr)
{
  if (m_hSocket == INVALID_SOCKET) return(false);

	#ifndef TARGET_WIN32
		socklen_t iSize;
	#else
		int iSize;
	#endif

	iSize= sizeof(sockaddr);
	bool ret = (getpeername(m_hSocket, (sockaddr *)pInetAddr, &iSize) != SOCKET_ERROR);
	if(!ret) ofxNetworkCheckError();
	return ret;
}

//--------------------------------------------------------------------------------
bool ofxTCPManager::GetInetAddr(LPINETADDR pInetAddr)
{
  if (m_hSocket == INVALID_SOCKET) return(false);

	#ifndef TARGET_WIN32
		socklen_t iSize;
	#else
		int iSize;
	#endif

	iSize= sizeof(sockaddr);
	bool ret = (getsockname(m_hSocket, (sockaddr *)pInetAddr, &iSize) != SOCKET_ERROR);
	if(!ret) ofxNetworkCheckError();
	return ret;
}

void ofxTCPManager::SetTimeoutConnect(int timeoutInSeconds) {
	m_dwTimeoutConnect= timeoutInSeconds;
}
void ofxTCPManager::SetTimeoutSend(int timeoutInSeconds) {
	m_dwTimeoutSend= timeoutInSeconds;
}
void ofxTCPManager::SetTimeoutReceive(int timeoutInSeconds) {
	m_dwTimeoutReceive= timeoutInSeconds;
}
void ofxTCPManager::SetTimeoutAccept(int timeoutInSeconds) {
	m_dwTimeoutAccept= timeoutInSeconds;
}
int ofxTCPManager::GetTimeoutConnect() {
	return m_dwTimeoutConnect;
}
int ofxTCPManager::GetTimeoutSend() {
	return m_dwTimeoutSend;
}
int ofxTCPManager::GetTimeoutReceive() {
	return m_dwTimeoutReceive;
}
int ofxTCPManager::GetTimeoutAccept() {
	return m_dwTimeoutAccept;
}

int ofxTCPManager::GetReceiveBufferSize() {
	if (m_hSocket == INVALID_SOCKET) return(false);

	#ifndef TARGET_WIN32
		socklen_t size;
	#else
		int size;
	#endif

	int sizeBuffer=0;
	size = sizeof(int);
	int ret = getsockopt(m_hSocket, SOL_SOCKET, SO_RCVBUF, (char*)&sizeBuffer, &size);
	if(ret==-1) ofxNetworkCheckError();
	return sizeBuffer;
}

bool ofxTCPManager::SetReceiveBufferSize(int sizeInByte) {
	if (m_hSocket == INVALID_SOCKET) return(false);

	if ( setsockopt(m_hSocket, SOL_SOCKET, SO_RCVBUF, (char*)&sizeInByte, sizeof(sizeInByte)) == 0){
		return true;
	}else{
		 ofxNetworkCheckError();
		return false;
	}
}

int ofxTCPManager::GetSendBufferSize() {
	if (m_hSocket == INVALID_SOCKET) return(false);

	#ifndef TARGET_WIN32
		socklen_t size;
	#else
		int size;
	#endif

	int sizeBuffer=0;
	size = sizeof(int);
	int ret = getsockopt(m_hSocket, SOL_SOCKET, SO_SNDBUF, (char*)&sizeBuffer, &size);
	if(ret==-1) ofxNetworkCheckError();
	return sizeBuffer;
}

bool ofxTCPManager::SetSendBufferSize(int sizeInByte) {
	if (m_hSocket == INVALID_SOCKET) return(false);

	if ( setsockopt(m_hSocket, SOL_SOCKET, SO_SNDBUF, (char*)&sizeInByte, sizeof(sizeInByte)) == 0){
		return true;
	}else{
		ofxNetworkCheckError();
		return false;
	}
}

int ofxTCPManager::GetMaxConnections() {
  return m_iMaxConnections;
}

bool ofxTCPManager::CheckHost(const char *pAddrStr) {
  hostent * hostEntry;
  in_addr iaHost;
  iaHost.s_addr = inet_addr(pAddrStr);
  hostEntry = gethostbyaddr((const char *)&iaHost, sizeof(struct in_addr), AF_INET);
  return ((!hostEntry) ? false : true);
}

