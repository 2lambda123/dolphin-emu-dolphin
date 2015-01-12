// Copyright 2013 Dolphin Emulator Project
// Licensed under GPLv2
// Refer to the license.txt file included.


#include "ReliableUnitTest.h"
#include "Common\MsgHandler.h"

ReliableUnitTest::ReliableUnitTest(sf::IpAddress adr, bool server)
	: m_adr(adr)
	, m_port(2660)
	, m_token(0)
	, m_recvToken(0)
	, m_controlSock()
{
	m_socket = std::make_shared<sf::UdpSocket>();
	m_socket->bind(2660);

	m_udp = std::make_shared<ReliableUDPConnection>(m_socket, m_adr, 2660);
	m_socket->setBlocking(true); 

	if (server)
	{
		sf::TcpListener listen;
		listen.listen(2760);

		listen.accept(m_controlSock);
	}
	else
	{
		m_controlSock.connect(adr, 2760);
	}

	sendStr[0][0] = "1";
	sendStr[0][1] = "2";
	sendStr[0][2] = "3";
	sendStr[0][3] = "4";
	sendStr[0][4] = "5";

	sendStr[1][0] = "A";
	sendStr[1][1] = "B";
	sendStr[1][2] = "C";
	sendStr[1][3] = "D";
	sendStr[1][4] = "E";


}

ReliableUnitTest::~ReliableUnitTest()
{
	m_socket->unbind();
}

void ReliableUnitTest::TestsServer()
{
	if (!TestConnectionS())
	{
		PanicAlertT("Failed");
		return;
	}
	SocketReset();

	/*if (!TestSendingRecievingS())
	{
		PanicAlertT("Failed");
		return;
	}
	SocketReset();
	*/
	if (!TestDroppingS())
	{
		PanicAlertT("Failed");
		return;
	}
	SocketReset();

	PanicAlertT("ALL ARE SUCCESSFUL");
}
void ReliableUnitTest::TestClient()
{
	if (!TestConnectionC())
	{
		PanicAlertT("Failed");
		return;
	}
	SocketReset();

	//if (!TestSendingRecievingC())
	//{
	//	PanicAlertT("Failed");
	//	return;
	//}
	//SocketReset();
	
	if (!TestDroppingC())
	{
		PanicAlertT("Failed");
		return;
	}
	SocketReset();

	PanicAlertT("ALL ARE SUCCESSFUL");
}

//---------------------------------------------------------
bool ReliableUnitTest::TestConnectionS()
{
	sf::Packet rpac;
	Recieve(rpac);
	sf::Packet pack;
	std::string str = "Hello Back";
	pack << str;
	Send(pack);

	std::string ret;
	rpac >> ret;

	if (ret == "Hello World")
	{
		PanicAlertT("Test1 Server Good");
		return true;
	}

	return false;
}
bool ReliableUnitTest::TestConnectionC()
{
	Common::Timer clk;
	clk.Start();
	while (clk.GetTimeElapsed() < 50)
	{

	}
	clk.Stop();
	std::string hello = "Hello World";
	sf::Packet pack;
	pack << hello;
	Send(pack);

	sf::Packet rpac;
	Recieve(rpac);

	std::string ret;
	rpac >> ret;
	if (ret == "Hello Back")
	{
		PanicAlertT("Test1 Client Good");
		return true;
	}

	return false;
}
//----------------------------------------------------------------------------------
bool ReliableUnitTest::TestSendingRecievingS()
{
	int expectedNumber = 1;
	sf::Packet rpac;

	int i = 1;
	while (i < UINT16_MAX + 50)
	{

		sf::Packet rpac;
		Recieve(rpac);
		int tmp = 0;
		rpac >> tmp;
		if (tmp == expectedNumber)
		{
			++expectedNumber;
		}
		else
		{
			PanicAlertT("Did Not Recieve Expected Number");
			return false;
		}

		Send(i);
		++i;
	}

	return true;
}
bool ReliableUnitTest::TestSendingRecievingC()
{

	int expectedNumber = 1;
	Common::Timer clk;
	clk.Start();
	while (clk.GetTimeElapsed() < 50)
	{

	}
	clk.Stop();

	int i = 1;
	while (i < UINT16_MAX + 50)
	{
		Send(i);
		sf::Packet rpac;
		Recieve(rpac);
		int tmp = 0;
		rpac >> tmp;
		if (tmp == expectedNumber)
		{
			++expectedNumber;
		}
		else
		{
			PanicAlertT("Did Not Recieve Expected Number");
			return false;
		}
		++i;
	}

	return true;
}

bool CheckBit(int index, int bitField)
{
	int test = 1;
	return ((bitField & (test << index)) != 0);
}
//----------------------------------------------------------------------------------
bool ReliableUnitTest::TestDroppingS()
{
	//SERVER TEST WILL DROP MESSAGES ACCORDING TO BITFIELD server Drop
	//Test all combination of drops withen the range of our missing bit field
	int numOfBits = 9;

	sf::Packet dropNumbers;
	dropNumbers << "Drop";
	int clientD = 255;
	int serverD = 255;
	dropNumbers << clientD;
	dropNumbers << serverD;
	m_controlSock.send(dropNumbers);

	PanicAlertT("Client %d, Server %d", clientD, serverD);

	for (int clientDrop = clientD; clientDrop >= 0; --clientDrop)
	{
		//This is for quick tests
		serverD = clientDrop;
		for (int serverDrop = serverD; serverDrop >= 0; --serverDrop)
		{
			sf::Packet ready;
			ready << "Ready";
			m_controlSock.send(ready);

			int expectedNumber = 0;
			//PanicAlert("Server Ready");
			for (int i = 0; i < numOfBits; ++i)
			{
				// -- drop or recieve
				if (CheckBit(i, serverDrop))
				{
					sf::Packet rPack;
					sf::Socket::Status stat;
					while (true)
					{
						stat = m_socket->receive(rPack, m_adr, m_port);
						m_udp->Receive(rPack);
						if (stat == sf::Socket::Done)
							break;

					}
				}
				else
				{
					RecieveDrop();
				}

				// -- grab any packets we recieved
				sf::Packet gpac;
				if (m_udp->GrabMessage(gpac))
				{
					int tmp;
					gpac >> tmp;
					if (tmp == expectedNumber)
					{
						++expectedNumber;
					}
					else
					{
						PanicAlertT("Did Not Recieve Expected Number( %u), got %u for test C %d, S %d", expectedNumber, tmp, clientDrop, serverDrop);
						return false;
					}
				}

				// -- send
				Send(i);
			}

			// -- grab any missing bits
			while (expectedNumber != numOfBits)
			{
			
				Send(50);

				sf::Packet rpac;
				if (Recieve(rpac))
				{
					int tmp = 0;
					rpac >> tmp;
					if (tmp == expectedNumber)
					{
						++expectedNumber;
					}
					else
					{
						PanicAlertT("Did Not Recieve Expected Number( %u), got %u for test C %d, S %d", expectedNumber, tmp, clientDrop,serverDrop);
						//return false;
					}
				}
				
			}// -- end of the drop loop



			// -- send to the client we are done
			Common::Timer clk2;
			clk2.Start();
			while (clk2.GetTimeElapsed() < 200)
			{

			}
			clk2.Stop();
			Send(100);

			// -- make sure client gets all his missing messages
			int done = 0;
			do
			{
				Send(55);
				sf::Packet rpac;
				Recieve(rpac);

				rpac >> done;
			} while (done != 100);

			// -- send to the client we are done
			Common::Timer clk3;
			clk3.Start();
			while (clk3.GetTimeElapsed() < 200)
			{

			}
			clk3.Stop();

			// -- empty out recieve
			sf::Socket::Status st = sf::Socket::NotReady;
			m_socket->setBlocking(false);
			do
			{
				sf::Packet rpack;
				sf::IpAddress ip;
				unsigned short prt;
				sf::Socket::Status st = m_socket->receive(rpack, ip, prt);
			} while (st == sf::Socket::Done);
			m_socket->setBlocking(true);

			// -- reset socket
			SocketReset();
			Common::Timer clk;
			clk.Start();
			while (clk.GetTimeElapsed() < 10)
			{

			}
			clk.Stop();
		}
	}
	sf::Packet ready;
	ready << "Ready";
	m_controlSock.send(ready);
	PanicAlert("Done Server");
	return true;
}


bool ReliableUnitTest::TestDroppingC()
{
	
	sf::Packet dropNumbers;
	int clientD = 0;
	int serverD = 0;

	m_controlSock.receive(dropNumbers);
	std::string c;
	
	dropNumbers >> c;
	if (c == "Drop")
	{
		dropNumbers >> clientD;
		dropNumbers >> serverD;
	}
	else
	{
		PanicAlertT("Didnt recieve our for loop numbers");
		return false;
	}

	PanicAlertT("Client %d, Server %d", clientD, serverD);

	//CLIENT ALL MESSAGES SENT RELIABLY
	sf::Packet ready;
	m_controlSock.receive(ready);
	std::string str;
	ready >> str;
	if (str != "Ready")
	{
		PanicAlertT("ERROR FROM CONTROL SOCKET");
		return false;
	}
	int numOfBits = 9;
	for (int clientDrop = clientD; clientDrop >= 0; --clientDrop)
	{
		//This is for quick tests
		serverD = clientDrop;
		for (int serverDrop = serverD; serverDrop >= 0; --serverDrop)
		{
			// -- Run Drops
			// -- send to the client we are done
			Common::Timer clk1;
			clk1.Start();
			while (clk1.GetTimeElapsed() < 200)
			{

			}
			clk1.Stop();
			//--empty out recieve
			sf::Socket::Status st = sf::Socket::NotReady;
			m_socket->setBlocking(false);
			do
			{
				sf::Packet rpack;
				sf::IpAddress ip;
				unsigned short prt;
				sf::Socket::Status st = m_socket->receive(rpack, ip, prt);
			} while (st == sf::Socket::Done);
			m_socket->setBlocking(true);

			int expectedNumber = 0;
			for (int i = 0; i < numOfBits; ++i)
			{
				
				// -- send
				Send(i);

				// -- drop or recieve
				if (CheckBit(i, clientDrop))
				{
					sf::Packet rPack;
					sf::Socket::Status stat;
					while (true)
					{
						stat = m_socket->receive(rPack, m_adr, m_port);
						m_udp->Receive(rPack);
						if (stat == sf::Socket::Done)
							break;

					}
				}
				else
				{
					RecieveDrop();
				}

				// -- grab any packets we recieved
				sf::Packet gpac;
				if (m_udp->GrabMessage(gpac))
				{
					int tmp;
					gpac >> tmp;
					if (tmp == expectedNumber)
					{
						++expectedNumber;
					}
					else
					{
						PanicAlertT("Did Not Recieve Expected Number( %u), got %u for test C %d, S %d", expectedNumber, tmp, clientDrop, serverDrop);
						return false;
					}
				}

				
			}// end of the drop loop
			
			//--grab any missing numbers
			while (expectedNumber != numOfBits)
			{
				Send(60);
				sf::Packet rpac;

				if (Recieve(rpac))
				{
					int tmp = 0;
					rpac >> tmp;
					if (tmp == expectedNumber)
					{
						++expectedNumber;
					}
					else
					{
						PanicAlertT("Did Not Recieve Expected Number( %u), got %u for test C %d, S %d", expectedNumber, tmp, clientDrop, serverDrop);
						return false;
					}
				}

			}
			
			//send to the Server we are done
			Common::Timer clk3;
			clk3.Start();
			while (clk3.GetTimeElapsed() < 200)
			{

			}
			clk3.Stop();
			Send(100);

			//--make sure server gets all his missing messages
			int done = 0;
			do
			{
				Send(66);
				sf::Packet rpac;
				Recieve(rpac);

				rpac >> done;
			} while (done != 100); 

			


			//--wait for the server to tell us that
			sf::Packet ready;
			m_controlSock.receive(ready);
			std::string str;
			ready >> str;
			if (str != "Ready")
			{
				PanicAlertT("ERROR FROM CONTROL SOCKET");
				return false;
			}
			//PanicAlert("Client Socket Resetting");
			SocketReset();
			Common::Timer clk;
			clk.Start();
			while (clk.GetTimeElapsed() < 10)
			{

			}
			clk.Stop();
		}
	}
	PanicAlert("Done Client");
	return true;
}


void ReliableUnitTest::UnreliableSentTest1()
{
	sf::Packet pack;
	pack << "Hello World";

	m_udp->SendUnreliable(pack);
}

void ReliableUnitTest::RecieveUnreliableSentTest1()
{
	sf::Packet pack;
	sf::Socket::Status stat = m_socket->receive(pack, m_adr, m_port);

	u8 header;
	u16 ack;
	std::string mess;
	
	pack >> header;
	pack >> ack;
	pack >> mess;
	
	PanicAlertT(mess.c_str());
}


//Must Send First (Currently set on Client)
void ReliableUnitTest::ReliableSentTest()
{
	int stringType=0;
	Send(sendStr[stringType]);//send 1
	Recieve();//recieve a
	
	

	Send(sendStr[stringType]);//send 2
	SendAck();//s ack 1
	SendAck();//s ack 2
	SendAck();//s ack 3
	Recieve();//recieve b
	
	Send(sendStr[stringType]);
	Recieve();
	
	Send(sendStr[stringType]);
	Recieve();
	
	Send(sendStr[stringType]);//send ack (will be +1 then the message they are expecting)
	Recieve();//will recieve missing bit field back with the missing message
	
	CheckString();
	//backup recieve we shouldn't be getting anything after this point
	do
	{
		
		m_udp->Send();
		Recieve();
	} while (true);

}

//Must Recieve First (Currently set on host)
void ReliableUnitTest::ReliableRecieveTest()
{
	int stringType = 1;
	Recieve(); //recieve 1
	Send(sendStr[stringType]);// send A

	RecieveDrop();//drop 2
	RecieveDrop();//recieve ack 1
	Recieve();//recieve ack 2
	RecieveDrop();//recieve ack 3

	Send(sendStr[stringType]);//send b
	
	Recieve();
	Send(sendStr[stringType]);
	
	Recieve();
	Send(sendStr[stringType]);
	
	Recieve();
	Send(sendStr[stringType]);

	//--back up recieve shouldn't recieve anything after this
	do
	{
		//Check();
		Recieve();
		m_udp->Send();	
	} while (true);

}

bool ReliableUnitTest::Recieve(sf::Packet& packet)
{
	sf::Packet rPack;
	sf::Socket::Status stat;
	while (true)
	{
		stat = m_socket->receive(rPack, m_adr, m_port);
		if (stat == sf::Socket::Done)
			break;
	}
	m_udp->Receive(rPack);
	return m_udp->GrabMessage(packet);
}


void ReliableUnitTest::Recieve()
{
	sf::Packet rPack;
	sf::Socket::Status stat;
	while (true)
	{
		stat = m_socket->receive(rPack, m_adr, m_port);
		if (stat == sf::Socket::Done)
			break;
	}
	bool is_mess = m_udp->Receive(rPack);

	sf::Packet packet;
	
	if (m_udp->GrabMessage(packet))
	{
		std::string mess;
		packet >> mess;
		
		if (!is_mess)
			mess.append("^");//we recieved an ack/old/future

		//PanicAlertT(mess.c_str());
		recvStr[m_recvToken] = mess;
	}
	else
	{
		std::string str = "_"; //nothing was in our recieved messages

		if (!is_mess)
			str.append("^");//we recieved an ack/old/future

		recvStr[m_recvToken] = str;
	}
	++m_recvToken;
}

void ReliableUnitTest::RecieveDrop()
{
	sf::Packet rPack;
	sf::Socket::Status stat;
	while (true)
	{
		stat = m_socket->receive(rPack, m_adr, m_port);
		if (stat == sf::Socket::Done)
			break;
	}
}

void ReliableUnitTest::Send(sf::Packet& packet, bool ack)
{
	m_udp->StoreSend(packet);
	m_udp->Send(ack);
}

void ReliableUnitTest::Send(int i, bool ack)
{
	
		sf::Packet packet;
		packet << i;
		m_udp->StoreSend(packet);
		m_udp->Send(ack);
	
}

void ReliableUnitTest::SendAck()
{
	Common::Timer clk;
	clk.Start();
	u64 wait = 21;
	while (clk.GetTimeElapsed() < 40)
	{

	}
	clk.Stop();
	m_udp->Send();
}

void ReliableUnitTest::Send(std::string str[MaxWords], bool ack)
{
	if (m_token < MaxWords)
	{
		sf::Packet packet;
		packet << str[m_token];
		m_udp->StoreSend(packet);
		m_udp->Send(ack);
		++m_token;
	}
	else
	{
		PanicAlertT("We Ran Out of words to send");
	}

}



void ReliableUnitTest::CheckString()
{
	std::string finalMess;

	for (int i = 0; i < m_recvToken; ++i)
	{
		finalMess.append(recvStr[i]);
		finalMess.append(" ");
	}

	finalMess.append("!!!");
	PanicAlertT(finalMess.c_str());
}

void  ReliableUnitTest::SocketReset()
{
	m_udp.reset();
	m_udp = std::make_shared<ReliableUDPConnection>(m_socket, m_adr, 2660);
}