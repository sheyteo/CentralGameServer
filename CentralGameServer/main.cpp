#include <iostream>
#include <vector>
#include <../asio/include/asio.hpp>


class Server
{
private:
	asio::io_context io_context;
	std::unique_ptr<asio::ip::udp::socket> socket;
	asio::ip::udp::endpoint remoteEndpoint;

	std::vector<char> buffer;
	asio::ip::udp::endpoint endpoint;
	

	void recieve()
	{
		
		socket->async_receive_from(asio::buffer(buffer.data(), buffer.size()), endpoint, std::bind(&Server::callback, this,
			std::placeholders::_1, std::placeholders::_2));
	}
	
	void callback(const asio::error_code& error, std::size_t bytes_transferred)
	{
		std::cout << "Msg from " << endpoint.address().to_string()
			<<":" << endpoint.port()<<"\n"<<"Msg-Content :" 
			<< std::string(buffer.data(),bytes_transferred) << "\n";
		recieve();
	}

	void send()
	{
		
	}
public:
	Server()
		:remoteEndpoint(asio::ip::udp::v4(), 6969),io_context()
	{
		buffer.resize(200);
		socket.reset(new asio::ip::udp::socket(io_context, remoteEndpoint));
		recieve();
		io_context.run();
	}
};

int main()
{
	Server server;
	return 0;
}