#ifndef SOCKET_HPP
# define SOCKET_HPP
# include <sys/socket.h>
# include <netinet/in.h>
# include <arpa/inet.h>
# include <string>
# include <poll.h>
# include <memory>

class Socket
{
    public:
        class sockException: public std::exception
        {
            private:
                std::string __description;
            public:
                sockException(Socket& sock, const std::string&& failedFunction, const std::string&& description="");
				sockException(Socket& sock, const std::string& failedFunction, const std::string&& description="");
                virtual const char* what(void) const noexcept(true);
        };
    public:
        enum struct sockType_t
        {
            TCP = 1
        };
        enum struct addrType_t
        {
            IPV4 = 2,
            IPV6 = 10
        };
		enum struct sockOption_t
		{
			NON_BLOCK = 1,
			REUSE_ADDR = 2
		};
	private:
		enum struct sockState_t
		{
			UNINITIALIZED,
			OK,
			FAILED,
			CLOSED
		};
		struct newSocket
		{
			int			fd;
			sockaddr_in	addrIn;
			socklen_t	addrInLen;
			sockType_t	sockType;
		};
    private:
		int         				__fd;
		struct pollfd				__polled[1];
		sockaddr_in 				__addrIn;
		socklen_t   				__addrInLen;
		sockType_t  				__socketType;
		addrType_t					__addrType;
		sockState_t					__state;
		int							__options;
	public:
		constexpr static ssize_t defaultChunkSize = 2048;
		constexpr static ssize_t defaultListenBacklog = SOMAXCONN;
	public:
		bool		ok(void) const noexcept(true);
		bool		closed(void) const noexcept(true);
		bool		pollin(void) noexcept(true);
		bool		pollout(void) noexcept(true);
	private:
		Socket(int __fd, sockType_t __socketType, addrType_t __addrType, sockaddr_in __addrIn, socklen_t __addrInLen);
		void    	__initSocket(void);
		void		__initAddrInStruct(const std::string& ipAddr, unsigned short port, addrType_t __type);
		void		__poll(short events);
		ssize_t		__peek(void);

		void		__updateState(sockState_t state);
		void		__checkState(sockState_t desiredState, const std::string&& function);

		template <typename T, typename U>
		struct isSame
		{
			static const bool value = false;
		};

		template <typename T>
		struct isSame<T, T>
		{
			static const bool value = true;
		};

		template <typename Checked, typename T>
		static bool		__is(void) noexcept(true);
	public:
		Socket(void);
		Socket(sockType_t __socketType, addrType_t __addrType);
		Socket(const Socket& cpy);
		Socket(const newSocket infos);
		~Socket(void) noexcept(false);

		void			open(void);
		void			close(void);

		void			setOptions(sockOption_t option, int value);

		void			bind(const std::string&& ipAddr, unsigned short port);
		void			bind(const std::string& ipAddr, unsigned short port);
		void			listen(int backlog=defaultListenBacklog);
		newSocket		accept(void);

		void			connect(const std::string& ipAddr, unsigned short port, addrType_t addrType=addrType_t::IPV4);
		void			connect(const std::string&& ipAddr, unsigned short port, addrType_t addrType=addrType_t::IPV4);
		void			connect(const Socket& peer);

		template <typename T>
		void			recv(T* buffer);

		std::string		recv(ssize_t n=defaultChunkSize);
		std::string		recvFrom(Socket& peer, ssize_t n=defaultChunkSize);

		template <typename T>
		void			sendTo(Socket& peer, T* data);

		void			sendTo(Socket& peer, const char* data);

		template <typename T>
		void			send(T* data);

		void			send(const char* data);

		operator		int(void) const;
		Socket&			operator=(const newSocket infos);
		Socket&			operator=(const Socket& cpy);
		bool			operator==(const Socket& b) const;
		bool			operator!=(const Socket& b) const;
};

template <typename T>
Socket&	operator<<(Socket& peer, const T&& data);

template <typename T>
Socket&	operator<<(T&& buffer, Socket& peer);

template <typename T>
Socket&	operator>>(Socket& peer, T&& buffer);

template <typename T>
Socket&	operator>>(T&& data, Socket& peer);

#include "Socket.tpp"

#endif