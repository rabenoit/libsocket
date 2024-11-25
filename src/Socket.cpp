#include "../include/Socket.hpp"
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>

/* ----------------------------------------------------------------- */
/* 					Exceptions for socket class:					 */
/* ----------------------------------------------------------------- */

Socket::sockException::sockException(Socket& sock, const std::string&& failedFunction, const std::string&& description)
{
	sock.__updateState(sockState_t::FAILED);
	int	currentErrno = errno;
	this->__description = "in 'Socket::" + failedFunction + "': ";
	this->__description += description.empty() ? strerror(currentErrno) : description;
	return ;
}

Socket::sockException::sockException(Socket& sock, const std::string& failedFunction, const std::string&& description)
{
	sock.__updateState(sockState_t::FAILED);
	int	currentErrno = errno;
	this->__description = "in '" + failedFunction + "': ";
	this->__description += description.empty() ? strerror(currentErrno) : description;
	return ;
}

const char*	Socket::sockException::what(void) const noexcept(true)
{
    return (this->__description.c_str());
}
/* ----------------------------------------------------------------- */


/* ----------------------------------------------------------------- */
/* 						Socket class constructors: 					 */
/* ----------------------------------------------------------------- */

// This is a private constructor used to instanciate new socket classes
// for accepted connections (using Socket::acept()). It doesn't call __initSocket()
// to get a new fd because we're using the one from Socket::accept().
Socket::Socket(int __fd, sockType_t __socketType, addrType_t __addrType, sockaddr_in __addrIn, socklen_t __addrInLen): __fd(__fd), __addrIn(__addrIn), __addrInLen(__addrInLen), __socketType(__socketType), __addrType(__addrType), __state(sockState_t::OK), __options(0)
{
	return ;
}

Socket::Socket(void): __fd(-1), __socketType(sockType_t::TCP), __addrType(addrType_t::IPV4), __state(sockState_t::CLOSED), __options(0)
{
	this->__initSocket();
    return ;
}

Socket::Socket(sockType_t __socketType, addrType_t __addrType): __fd(-1), __socketType(__socketType), __addrType(__addrType), __state(sockState_t::CLOSED), __options(0)
{
	this->__initSocket();
    return ;
}

Socket::Socket(const Socket& cpy)
{
	if (this == &cpy)
		return ;
	*this = cpy;
	return ;
}

Socket::Socket(const newSocket infos): __state(sockState_t::OK), __options(0)
{
	*this = infos;
	return ;
}

Socket::~Socket(void) noexcept(false)
{
	this->close();
	return ;
}

/* ----------------------------------------------------------------- */

void		Socket::open(void)
{
	this->__initSocket();
	return ;
}

void		Socket::close(void)
{
	if (this->__state != sockState_t::CLOSED)
		this->__updateState(sockState_t::CLOSED);
	return ;
}

void		Socket::__initSocket(void)
{
	this->__fd = socket((int)this->__addrType, (int)this->__socketType, 0);
	if (this->__fd == -1)
		throw sockException(*this, "Socket::__initSocket, socket");
	this->__updateState(sockState_t::UNINITIALIZED);
	return ;
}

void		Socket::__initAddrInStruct(const std::string& ipAddr, unsigned short port, addrType_t __addrType)
{
	memset(&this->__addrIn, 0, sizeof(this->__addrIn));
	this->__addrIn.sin_port = htons(port);
	if (inet_aton(ipAddr.c_str(), &this->__addrIn.sin_addr) == -1)
		throw sockException(*this, "Socket::__initAddrStruct, inet_aton");
	this->__addrInLen = sizeof(this->__addrIn);
	this->__addrIn.sin_family = (int)__addrType;
	return ;
}

void		Socket::__poll(short events)
{
	memset(&this->__polled[0], 0, sizeof(this->__polled));
	this->__polled[0].fd = this->__fd;
	this->__polled[0].events = events;
	if (poll(this->__polled, 1, 0) == -1 && errno != EINTR)
		throw sockException(*this, "__poll, poll");
	return ;
}

ssize_t		Socket::__peek(void)
{
	char	buff[1];

	memset(buff, 0, sizeof(buff));
	return (::recv(this->__fd, buff, sizeof(buff), MSG_PEEK | MSG_DONTWAIT));
}

void		Socket::__updateState(sockState_t state)
{
	if (state == sockState_t::CLOSED)
	{
		if (::close(this->__fd) == -1)
			throw sockException(*this, "__updateState, close");
	}
	this->__state = state;
	return ;
}

void		Socket::__checkState(sockState_t desiredState, const std::string&& function)
{
	if (desiredState == this->__state)
		return ;
	if (this->__state == sockState_t::CLOSED)
		throw sockException(*this, function, "impossible operation. Socket is not opened.");
	else if (this->__state == sockState_t::FAILED)
		throw sockException(*this, function, "impossible operation. Socket is in fail state.");
	return ;
}

void	Socket::setOptions(sockOption_t option, int value)
{
	this->__checkState(sockState_t::UNINITIALIZED, "setOptions");
	int*	valuePtr = &value;
	if (option == sockOption_t::NON_BLOCK)
	{
		int flags = fcntl(this->__fd, F_GETFL, 0);
		if (flags == -1)
			throw sockException(*this, "setOptions, fcntl");
		flags = value == true ? (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK);
		if (fcntl(this->__fd, F_SETFL, flags) == -1)
			throw sockException(*this, "setOptions, fcntl");
	}
	else if (setsockopt(this->__fd, SOL_SOCKET, (int)option, valuePtr, sizeof(valuePtr)) == -1)
		throw sockException(*this, "setOptions, setsockopt");
	this->__options = value ? (this->__options | (int)option) : (this->__options & ~(int)option);
	return ;
}

void	Socket::bind(const std::string&& ipAddr, unsigned short port)
{
	this->__checkState(sockState_t::UNINITIALIZED, "Socket::bind");
	this->__initAddrInStruct(ipAddr, port, this->__addrType);
	if (::bind(this->__fd, (sockaddr*)&this->__addrIn, this->__addrInLen) == -1)
		throw sockException(*this, "bind, bind");
	this->__updateState(sockState_t::OK);
	return ;
}

void	Socket::bind(const std::string& ipAddr, unsigned short port)
{
	this->__checkState(sockState_t::UNINITIALIZED, "bind");
	this->__initAddrInStruct(ipAddr, port, this->__addrType);
	if (::bind(this->__fd, (sockaddr*)&this->__addrIn, this->__addrInLen) == -1)
		throw sockException(*this, "bind, bind");
	this->__updateState(sockState_t::OK);
	return ;
}

bool	Socket::ok(void) const noexcept(true)
{
	return (this->__state == sockState_t::OK);
}

bool	Socket::closed(void) const noexcept(true)
{
	return (this->__state == sockState_t::CLOSED);
}

bool	Socket::pollin(void) noexcept(true)
{
	if (!this->ok())
		return (false);
	if (this->__peek() == 0)
	{
		this->__updateState(sockState_t::CLOSED);
		return (false);
	}
	this->__poll(POLLIN);
	return (this->ok() && (this->__polled[0].revents & POLLIN));
}

bool	Socket::pollout(void) noexcept(true)
{
	if (!this->ok())
		return (false);
	if (this->__peek() == 0)
	{
		this->__updateState(sockState_t::CLOSED);
		return (false);
	}
	this->__poll(POLLOUT);
	return (this->ok() && (this->__polled[0].revents & POLLOUT));
}

void	Socket::listen(int backlog)
{
	this->__checkState(sockState_t::OK, "listen");
	if (::listen(this->__fd, backlog) == -1)
		throw sockException(*this, "listen, listen");
	return ;
}

Socket::newSocket		Socket::accept(void)
{
	this->__checkState(sockState_t::OK, "accept");
	newSocket	newConn;

	memset(&newConn, 0, sizeof(newConn));
	newConn.sockType = this->__socketType;
	newConn.fd = ::accept(this->__fd, (sockaddr*)&newConn.addrIn, &newConn.addrInLen);
	if (newConn.fd == -1)
		throw sockException(*this, "accept, accept");
	return (newConn);
}

void		Socket::connect(const std::string& ipAddr, unsigned short port, addrType_t addrType)
{
	this->__checkState(sockState_t::UNINITIALIZED, "connect");
	this->__initAddrInStruct(ipAddr, port, addrType);
	if (::connect(this->__fd, (sockaddr*)&this->__addrIn, this->__addrInLen) == -1)
		throw sockException(*this, "connect, connect");
	this->__updateState(sockState_t::OK);
	return ;
}

void		Socket::connect(const std::string&& ipAddr, unsigned short port, addrType_t addrType)
{
	this->connect(ipAddr, port, addrType);
	return ;
}

void		Socket::connect(const Socket& peer)
{
	this->__checkState(sockState_t::UNINITIALIZED, "connect");
	if (::connect(this->__fd, (sockaddr*)&peer.__addrIn, peer.__addrInLen) == -1)
		throw sockException(*this, "connect, connect");
	this->__updateState(sockState_t::OK);
	return ;
}

std::string	Socket::recvFrom(Socket& peer, ssize_t n)
{
	this->__checkState(sockState_t::OK, "recvFrom");
	return (peer.recv(n));
}

std::string	Socket::recv(ssize_t n)
{
	this->__checkState(sockState_t::OK, "recv");
	std::string	buffer;
	ssize_t		recvdBytes = 0;

	buffer.resize(n);
	recvdBytes = ::recv(this->__fd, buffer.data(), n, 0);
	if (recvdBytes == 0 || (recvdBytes == -1 && errno == ECONNRESET))
		this->__updateState(sockState_t::CLOSED);
	else if (recvdBytes == -1 && errno != EWOULDBLOCK)
		throw sockException(*this, "recv, recv");
	return (buffer);
}

void	Socket::sendTo(Socket& peer, const char* data)
{
	this->__checkState(sockState_t::OK, "sendTo");
	peer.send(data);
	return ;
}

void	Socket::send(const char* data)
{
	this->__checkState(sockState_t::OK, "send");
	ssize_t	sentBytes = ::send(this->__fd, data, strlen(data), 0);
	if (sentBytes == 0 || (sentBytes == -1 && errno == ECONNRESET))
		this->__updateState(sockState_t::CLOSED);
	else if (sentBytes == -1 && errno != EWOULDBLOCK)
		throw sockException(*this, "send, send");
	return ;
}

Socket::operator int(void) const
{
	return (this->__fd);
}

Socket&	Socket::operator=(const newSocket infos)
{
	this->__fd = infos.fd;
	this->__addrIn = infos.addrIn;
	this->__addrType = (addrType_t)infos.addrIn.sin_family;
	this->__socketType = infos.sockType;
	this->__state = infos.fd == -1 ? sockState_t::FAILED : sockState_t::OK;
	this->__options = 0;
	return (*this);
}

Socket&	Socket::operator=(const Socket& cpy)
{
	if (this == &cpy)
		return (*this);
	this->__addrIn = cpy.__addrIn;
	this->__addrInLen = cpy.__addrInLen;
	this->__addrType = cpy.__addrType;
	this->__fd = cpy.__fd;
	this->__options = cpy.__options;
	this->__socketType = cpy.__socketType;
	this->__state = cpy.__state;
	return (*this);
}

bool	Socket::operator==(const Socket& b) const
{
	return (this->__fd == b.__fd);
}

bool	Socket::operator!=(const Socket& b) const
{
	return (this->__fd != b.__fd);
}
