#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <iostream>
#include <string>
#include <cmath>


const float MOVE_SPEED = 200;
sf::IpAddress serverIp;
const int port = 50001;

sf::TcpSocket TcpSocket;

sf::Mutex globalMutex;
sf::Packet serverPacket;
sf::Packet clientPacket;

bool quit = false;
bool clientConnected = false;

std::string h_command = "";
float h_offset_x = 0;
float h_offset_y = 0;
float h_DiskX;
float h_DiskY;
float h_Angle;

std::string c_command = "";
float c_offset_x = 0;
float c_offset_y = 0;
float c_DiskX;
float c_DiskY;
float c_Angle;

const float pi = 3.14159f;
const int gameWidth = 800;
const int gameHeight = 600;
float diskRadius = 10.f;
const float diskSpeed = 200.f;
float diskAngle = 0.f; 

//TCP functions
void TCPServerListener()
{
	std::cout << "Listening for clients." << std::endl;
	sf::TcpListener listener;
	listener.listen(port);
	listener.accept(TcpSocket);
	std::cout << "New client connected: " << TcpSocket.getRemoteAddress() << std::endl;
	clientConnected = true;
	std::string command;
	float offset_x;
	float offset_y;
	float d_x;
	float d_y;
	float d_a;
	while (!quit)
	{
		TcpSocket.receive(clientPacket);

		while (!clientPacket.endOfPacket())
		{
			globalMutex.lock();
			clientPacket >> command >> offset_x >> offset_y >> d_x >> d_y >> d_a;
			c_command = command;
			c_offset_x = offset_x;
			c_offset_y = offset_y;
			c_DiskX = d_x;
			c_DiskY = d_y;
			c_Angle = d_a;
			globalMutex.unlock();
		}

		clientPacket.clear();
	}
}

void TCPClientListener()
{
	std::string command;
	float offset_x;
	float offset_y;
	float d_x;
	float d_y;
	float d_a;

	while (!quit)
	{
		TcpSocket.receive(serverPacket);

		while (!serverPacket.endOfPacket())
		{
			globalMutex.lock();
			serverPacket >> command >> offset_x >> offset_y >> d_x >> d_y >> d_a;
			h_command = command;
			h_offset_x = offset_x;
			h_offset_y = offset_y;
			h_DiskX = d_x;
			h_DiskY = d_y;
			h_Angle = d_a;
			globalMutex.unlock();
		}

		serverPacket.clear();
	}
}

void TCPServerSend()
{
	if (serverPacket.getDataSize() > 0)
	{
		globalMutex.lock();
		TcpSocket.send(serverPacket);
		serverPacket.clear();
		globalMutex.unlock();
	}
}

void TCPClientSend()
{
	if (clientPacket.getDataSize() > 0)
	{
		globalMutex.lock();
		TcpSocket.send(clientPacket);
		clientPacket.clear();
		globalMutex.unlock();
	}
}

int main()
{
	char mode = 'n';

	std::string str_mode;

	while (!(mode == 'C' || mode == 'S' || mode == 'c' || mode == 's'))
	{
		std::cout << "[S]erver or [C]lient?\n";
		std::cin >> mode;

		if (!(mode == 'C' || mode == 'S' || mode == 'c' || mode == 's'))
		{
			std::cout << "Invalid." << std::endl;
		}
	}
	if (mode == 'C' || mode == 'c') 
	{
		str_mode = "client";
		do
		{
			std::cout << "Type the address or name of the server to connect to: ";
			std::cin >> serverIp;
		} while (serverIp == sf::IpAddress::None);
	}
	if (mode == 'S' || mode == 's')
	{
		str_mode = "server";
	}
	while (!(mode == 'C' || mode == 'S' || mode == 'c' || mode == 's'))
	{
		std::cout << "[S]erver or [C]lient?\n";
		std::cin >> mode;

		if (!(mode == 'C' || mode == 'S' || mode == 'c' || mode == 's'))
		{
			std::cout << "Invalid." << std::endl;
		}
	}

	std::cout << "Selected mode: " << str_mode << std::endl;
	
	sf::Thread TCPserverListenerThread(&TCPServerListener);
	sf::Thread TCPclientListenerThread(&TCPClientListener);

	sf::Clock Clock;
	sf::Clock PacketClock;
	int packet_counter = 0;

	sf::Time time;

	// Creating Player 1
	sf::Texture m_texture1;
	sf::Sprite server_sprite;
	m_texture1.loadFromFile("HockeyRed.png");
	server_sprite.setTexture(m_texture1);
	server_sprite.setPosition(50, 250);

	//Creating Player 2
	sf::Texture m_texture2;
	sf::Sprite client_sprite;
	m_texture2.loadFromFile("HockeyGreen.png");
	client_sprite.setTexture(m_texture2);
	client_sprite.setPosition(700, 250);

	//Creating Background
	sf::Texture backgroundTexture;
	sf::Sprite bgSprite;
	backgroundTexture.loadFromFile("background.png");
	bgSprite.setTexture(backgroundTexture);

	// Creating Disk
	sf::Texture diskTexture;
	sf::Sprite disk;
	diskTexture.loadFromFile("disk.png");
	disk.setTexture(diskTexture);
	disk.setOrigin(diskRadius / 2, diskRadius / 2);

	int left_over_x = 0;
	int left_over_y = 0;
	float offset_x = 0;
	float offset_y = 0;

	int d_left_over_x = 0;
	int d_left_over_y = 0;
	float d_offset_x = 0;
	float d_offset_y = 0;

	std::string command = "";

	if (str_mode == "server")
	{
		TCPserverListenerThread.launch();
	}
	else
	{
			TcpSocket.connect(serverIp, port);
			TCPclientListenerThread.launch();
	}

	diskAngle = (20) * 2 * pi / 360;
	
	if (str_mode == "server") {
		while (!clientConnected) {}
	}
	
	sf::RenderWindow window(sf::VideoMode(gameWidth, gameHeight), "SFML " + str_mode + " over TCP");
	window.setFramerateLimit(60);

	while (window.isOpen())
	{
		sf::Event event;
		while (window.pollEvent(event))
		{
			if (event.type == sf::Event::Closed)
			{
				quit = true;
				if (str_mode == "server")
					TCPserverListenerThread.terminate();
				else
					TCPclientListenerThread.terminate();				
				window.close();
			}
		}
		if (str_mode == "server")
		{
			if (sf::Keyboard::isKeyPressed(sf::Keyboard::A) && server_sprite.getPosition().x > 0)
				server_sprite.move(-1 * MOVE_SPEED * time.asSeconds(), 0);

			if (sf::Keyboard::isKeyPressed(sf::Keyboard::S) && server_sprite.getPosition().y < gameHeight - server_sprite.getTexture()->getSize().y)
				server_sprite.move(0, 1 * MOVE_SPEED * time.asSeconds());

			if (sf::Keyboard::isKeyPressed(sf::Keyboard::D) && server_sprite.getPosition().x < gameWidth - server_sprite.getTexture()->getSize().x)
				server_sprite.move(1 * MOVE_SPEED * time.asSeconds(), 0);

			if (sf::Keyboard::isKeyPressed(sf::Keyboard::W) && server_sprite.getPosition().y > 0)
				server_sprite.move(0, -1 * MOVE_SPEED * time.asSeconds());
			
			if (c_command.compare("move") == 0)
			{
				globalMutex.lock();
				command = c_command;

				offset_x = c_offset_x - client_sprite.getPosition().x;
				offset_y = c_offset_y - client_sprite.getPosition().y;

				c_command = "";
				c_offset_x = 0;
				c_offset_y = 0;
				globalMutex.unlock();

				left_over_x = offset_x;
				left_over_y = offset_y;

			} //end of if c_command is "move"

			float move_offset = MOVE_SPEED * time.asSeconds();
			//interpolation
			if (left_over_x != 0)
			{
				if (left_over_x < 0)
				{
					if ((left_over_x + move_offset) < 0)
					{
						client_sprite.move(-1 * move_offset, 0);
						left_over_x += move_offset;
					}
					else
					{
						client_sprite.move(left_over_x, 0);
						left_over_x = 0;
					}
				}
				else if (left_over_x > 0)
				{
					if ((left_over_x - move_offset) > 0)
					{
						client_sprite.move(move_offset, 0);
						left_over_x -= move_offset;
					}
					else
					{
						client_sprite.move(left_over_x, 0);
						left_over_x = 0;
					}
				}
			}
			if (left_over_y != 0)
			{
				if (left_over_y < 0)
				{
					if ((left_over_y + move_offset) < 0)
					{
						client_sprite.move(0, -1 * move_offset);
						left_over_y += move_offset;
					}
					else
					{
						client_sprite.move(0, left_over_y);
						left_over_y = 0;
					}
				}
				else if (left_over_y > 0)
				{
					if ((left_over_y - move_offset) > 0)
					{
						client_sprite.move(0, move_offset);
						left_over_y -= move_offset;
					}
					else
					{
						client_sprite.move(0, left_over_y);
						left_over_y = 0;
					}
				}
			}
			//end of interpolation
		}
		else //client mode
		{

			if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left) && client_sprite.getPosition().x > 0)
				client_sprite.move(-1 * MOVE_SPEED * time.asSeconds(), 0);

			if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down) && client_sprite.getPosition().y < gameHeight - client_sprite.getTexture()->getSize().y)
				client_sprite.move(0, 1 * MOVE_SPEED * time.asSeconds());

			if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right) && client_sprite.getPosition().x < gameWidth - client_sprite.getTexture()->getSize().x)
				client_sprite.move(1 * MOVE_SPEED * time.asSeconds(), 0);

			if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up) && client_sprite.getPosition().y > 0)
				client_sprite.move(0, -1 * MOVE_SPEED * time.asSeconds());

			if (h_command.compare("move") == 0)
			{
				globalMutex.lock();
				command = h_command;

				offset_x = h_offset_x - server_sprite.getPosition().x;
				offset_y = h_offset_y - server_sprite.getPosition().y;
				
				d_offset_x = h_DiskX - disk.getPosition().x;
				d_offset_x = h_DiskY - disk.getPosition().y;
				disk.setPosition(h_DiskX, h_DiskY);
				diskAngle = h_Angle;
				
				h_command = "";
				h_offset_x = 0;
				h_offset_y = 0; 
				h_DiskX = 0;
				h_DiskY = 0;
				globalMutex.unlock();

				left_over_x = offset_x;
				left_over_y = offset_y;

				d_left_over_x = d_offset_x;
				d_left_over_x = d_offset_x;


			} //end of if h_command is "move"

			//interpolation of disk
			float d_move_offset = diskSpeed * time.asSeconds();
			if (d_left_over_x != 0)
			{
				if (d_left_over_x < 0)
				{
					if ((d_left_over_x + d_move_offset) < 0)
					{
						disk.move(-1 * d_move_offset, 0);
						d_left_over_x += d_move_offset;
					}
					else
					{
						disk.move(d_left_over_x, 0);
						d_left_over_x = 0;
					}
				}
				else if (d_left_over_x > 0)
				{
					if ((d_left_over_x - d_move_offset) > 0)
					{
						disk.move(d_move_offset, 0);
						d_left_over_x -= d_move_offset;
					}
					else
					{
						disk.move(d_left_over_x, 0);
						d_left_over_x = 0;
					}
				}
			}
			
			if (d_left_over_y != 0)
			{
				if (d_left_over_y < 0)
				{
					if ((d_left_over_y + d_move_offset) < 0)
					{
						disk.move(0, -1 * d_move_offset);
						d_left_over_y += d_move_offset;
					}
					else
					{
						disk.move(0, d_left_over_y);
						d_left_over_y = 0;
					}
				}
				else if (d_left_over_y > 0)
				{
					if ((d_left_over_y - d_move_offset) > 0)
					{
						disk.move(0, d_move_offset);
						d_left_over_y -= d_move_offset;
					}
					else
					{
						disk.move(0, d_left_over_y);
						d_left_over_y = 0;
					}
				}
			}
			//end of interpolation of disk
			// interpolation of player 1
			float move_offset = MOVE_SPEED * time.asSeconds();
			if (left_over_x != 0)
			{
				if (left_over_x < 0)
				{
					if ((left_over_x + move_offset) < 0)
					{
						server_sprite.move(-1 * move_offset, 0);
						left_over_x += move_offset;
					}
					else
					{
						server_sprite.move(left_over_x, 0);
						left_over_x = 0;
					}
				}
				else if (left_over_x > 0)
				{
					if ((left_over_x - move_offset) > 0)
					{
						server_sprite.move(move_offset, 0);
						left_over_x -= move_offset;
					}
					else
					{
						server_sprite.move(left_over_x, 0);
						left_over_x = 0;
					}
				}
			}
			if (left_over_y != 0)
			{
				if (left_over_y < 0)
				{
					if ((left_over_y + move_offset) < 0)
					{
						server_sprite.move(0, -1 * move_offset);
						left_over_y += move_offset;
					}
					else
					{
						server_sprite.move(0, left_over_y);
						left_over_y = 0;
					}
				}
				else if (left_over_y > 0)
				{
					if ((left_over_y - move_offset) > 0)
					{
						server_sprite.move(0, move_offset);
						left_over_y -= move_offset;
					}
					else
					{
						server_sprite.move(0, left_over_y);
						left_over_y = 0;
					}
				}
			}
			// end of player 1 interpolation

		} //end of client mode if

		float factor = diskSpeed * time.asSeconds();
		disk.move(std::cos(diskAngle)* factor, std::sin(diskAngle)* factor);
		// Check collisions between the disk and the screen
		if (disk.getPosition().x - diskRadius < 0.f)
		{
			disk.setPosition(gameWidth / 2, gameHeight / 2);
		}
		if (disk.getPosition().x + diskRadius > gameWidth)
		{
			disk.setPosition(gameWidth / 2, gameHeight / 2);
		}
		if (disk.getPosition().y - diskRadius < 0.f)
		{
			diskAngle = -diskAngle;
			disk.setPosition(disk.getPosition().x, diskRadius + 0.1f);
		}
		if (disk.getPosition().y + diskRadius > gameHeight)
		{
			diskAngle = -diskAngle;
			disk.setPosition(disk.getPosition().x, gameHeight - diskRadius - 0.1f);
		}

		// Check the collisions between the disk and the players
		// Player 1
		if (disk.getPosition().x - diskRadius < server_sprite.getPosition().x + server_sprite.getTexture()->getSize().x / 2 &&
			disk.getPosition().x - diskRadius > server_sprite.getPosition().x &&
			disk.getPosition().y + diskRadius >= server_sprite.getPosition().y - server_sprite.getTexture()->getSize().y / 2 &&
			disk.getPosition().y - diskRadius <= server_sprite.getPosition().y + server_sprite.getTexture()->getSize().y / 2)
		{
			if (disk.getPosition().y > server_sprite.getPosition().y)
				diskAngle = pi - diskAngle + (10) * pi / 180;
			else
				diskAngle = pi - diskAngle - (10) * pi / 180;

			disk.setPosition(server_sprite.getPosition().x + diskRadius + server_sprite.getTexture()->getSize().x / 2 + 0.1f, disk.getPosition().y);
		}

		// Player 2
		if (disk.getPosition().x + diskRadius > client_sprite.getPosition().x - client_sprite.getTexture()->getSize().x / 2 &&
			disk.getPosition().x + diskRadius < client_sprite.getPosition().x &&
			disk.getPosition().y + diskRadius >= client_sprite.getPosition().y - client_sprite.getTexture()->getSize().y / 2 &&
			disk.getPosition().y - diskRadius <= client_sprite.getPosition().y + client_sprite.getTexture()->getSize().y / 2)
		{
			if (disk.getPosition().y > client_sprite.getPosition().y)
				diskAngle = pi - diskAngle + (10) * pi / 180;
			else
				diskAngle = pi - diskAngle - (10) * pi / 180;
			disk.setPosition(client_sprite.getPosition().x - diskRadius - client_sprite.getTexture()->getSize().x / 2 - 0.1f, disk.getPosition().y);
		}

		window.draw(bgSprite);
		window.draw(client_sprite);
		window.draw(server_sprite);
		window.draw(disk);
		window.display();

		if (str_mode == "server")
		{
			if (PacketClock.getElapsedTime().asSeconds() >= 1)
			{
				packet_counter = 0;
				PacketClock.restart();
			}
			else if ((PacketClock.getElapsedTime().asMilliseconds() / 33) > packet_counter)
			{
				serverPacket << "move" << server_sprite.getPosition().x << server_sprite.getPosition().y << disk.getPosition().x << disk.getPosition().y << diskAngle;
				TCPServerSend();
				
				packet_counter++;
			}
		}
		else
		{
			if (PacketClock.getElapsedTime().asSeconds() >= 1)
			{
				packet_counter = 0;
				PacketClock.restart();
			}
			else if ((PacketClock.getElapsedTime().asMilliseconds() / 33) > packet_counter)
			{
				clientPacket << "move" << client_sprite.getPosition().x << client_sprite.getPosition().y << disk.getPosition().x << disk.getPosition().y << diskAngle;
				TCPClientSend();
				packet_counter++;
			}
		}

		time = Clock.restart();
	}
}