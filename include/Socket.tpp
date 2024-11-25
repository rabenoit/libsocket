#ifndef SOCKET_TPP
# define SOCKET_TPP
# include "Socket.hpp"
# include <iostream>
# include <vector>

template <typename Checked, typename T>
bool	Socket::__is(void) noexcept(true)
{
	return (isSame<Checked, T>::value);
}

template <typename T>
void	Socket::recv(T* buffer)
{
	this->__checkState(sockState_t::OK, "recv");
	ssize_t		recvdBytes = 0;

	recvdBytes = ::recv(this->__fd, buffer, sizeof(*buffer), 0);
	if (recvdBytes == 0 || (recvdBytes == -1 && errno == ECONNRESET))
		this->__updateState(sockState_t::CLOSED);
	else if (recvdBytes == -1 && errno != EWOULDBLOCK)
		throw sockException(*this, "recv, recv");
	return ;
}

template <typename T>
void	Socket::sendTo(Socket& peer, T* data)
{
	this->__checkState(sockState_t::OK, "sendTo");
	peer.send(data);
	return ;
}

template <typename T>
void	Socket::send(T* data)
{
	this->__checkState(sockState_t::OK, "send");
	ssize_t	sentBytes = ::send(this->__fd, data, sizeof(*data), 0);
	if (sentBytes == 0 || (sentBytes == -1 && errno == ECONNRESET))
		this->__updateState(sockState_t::CLOSED);
	else if (sentBytes == -1 && errno != EWOULDBLOCK)
		throw sockException(*this, "send, send");
	return ;
}

template <typename T>
Socket&		operator<<(Socket& peer, const T&& data)
{
	peer.send(data);
	return (peer);
}

template <typename T>
Socket&	operator<<(T&& buffer, Socket& peer)
{
	peer.recv(buffer);
	return (peer);
}

template <typename T>
Socket&	operator>>(Socket& peer, T&& buffer)
{
	peer.recv(buffer);
	return (peer);
}

template <typename T>
Socket&	operator>>(T&& buffer, Socket& peer)
{
	peer.send(buffer);
	return (buffer);
}

#endif