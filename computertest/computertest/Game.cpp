#pragma once
#include "Game.h"
#include <iostream>
#include <vector>
#include <Windows.h>

//====================================================================================
//================================ CONSTRUCTOR =======================================
//====================================================================================
Game::Game(const Images &images, Uint32 image_id)
	:m_me(std::make_unique<MyPlayer>()),
	m_background(images[int(BACKGROUND)])
{
	if (m_socket.connect(sf::IpAddress::LocalHost, 5555) != sf::TcpSocket::Done)
		//if (m_socket.connect("10.2.15.207", 5555) != sf::TcpSocket::Done)
		std::cout << "no connecting\n";

	sf::Packet packet;
	packet << image_id; //����� ���� �� ������ ���
	if (m_socket.send(packet) != sf::TcpSocket::Done)
		std::cout << "no sending image\n";

	receive(images);//����� ���� �����
}
//--------------------------------------------------------------------------
void Game::receive(const Images &images)
{
	sf::Packet packet;
	std::pair <Uint32, sf::Vector2f> temp;
	Uint32 image;
	float radius;

	m_socket.setBlocking(true);//**********************************
	Sleep(100);//*********************************

	auto status = m_socket.receive(packet);

	if (status == sf::TcpSocket::Done)
	{
		while (!packet.endOfPacket())//����� �� �� ������ ��� ����
		{
			//std::cout << "while\n";
			packet >> temp;

			if (temp.first >= 1000 && temp.first <= 10000)
				m_objectsOnBoard.insert(temp);

			else if (temp.first >= 200 && temp.first <= 300)//???????????????????????
			{
				packet >> radius >> image;
				m_players.emplace(temp.first, std::make_unique<OtherPlayers>(temp.first, images[image], radius, temp.second));
			}
		}
	}

	m_players.erase(temp.first); //����� ������ ��� �������� ������

	m_me->setId(temp.first);//����� ������ ���
	m_me->setTexture(images[image]);
	m_me->setPosition(temp.second);
	m_me->setCenter(temp.second + Vector2f{ NEW_PLAYER,NEW_PLAYER });
}

//====================================================================================
//================================     PLAY     ======================================
//====================================================================================
unsigned Game::play(sf::RenderWindow &w, const Images &images)
{
	m_socket.setBlocking(false);

	sf::Packet packet;
	while (true)
	{
		auto speed = TimeClass::instance().RestartClock();

		//����� �� �����
		if (!updateMove(speed))
			return m_me->getScore();

		//���� ���� �����
		if (!receiveChanges(images))
			return m_me->getScore();

		draw(w);
	}


	return 0;
}

//====================================================================================
//===========================      UPDATE MOVE       =================================
//====================================================================================
//����� ��� �� ����
bool Game::updateMove(float speed)
{
	sf::Packet packet;
	packet.clear();
	bool temp = true;

	move(speed);

	std::vector<Uint32> deleted;

	temp = m_me->collision(deleted, m_objectsOnBoard, m_players, m_me.get());
	packet << m_me->getId() << m_me->getRadius() << m_me->getPosition() << deleted;

	if (m_socket.send(packet) != sf::TcpSocket::Done)
		std::cout << "no sending data\n";


	return temp;
}
//--------------------------------------------------------------------------
//==========================================================================
void Game::move(float speed) {

	std::cout << "move f\n";
	float max = -1, tempR;
	sf::Vector2f temp;

	temp = { m_me->getCenter().x , m_me->getCenter().y - speed*MOVE };
	tempR = direction(temp);
	if (tempR > max) {// up
		std::cout << tempR <<" up\n";
		max = tempR;
		m_me->setCenter(temp);
	}
	temp = { m_me->getCenter().x , m_me->getCenter().y + speed*MOVE };
	tempR = direction(temp);
	if (tempR > max) {// down
		max = tempR;
		m_me->setCenter(temp);
		std::cout <<tempR << " down\n";

	}
	temp = { m_me->getCenter().x - speed*MOVE , m_me->getCenter().y };
	tempR = direction(temp);
	if (tempR > max) {// right
		max = tempR;
		m_me->setCenter(temp);
		std::cout << tempR << " right\n";

	}
	temp = { m_me->getCenter().x + speed*MOVE  , m_me->getCenter().y };
	tempR = direction(temp);
	if (tempR > max) {// left
		max = tempR;
		m_me->setCenter(temp);
		std::cout << tempR << " left\n";

	}
	///Sleep(500);

}
//==========================================================================================================
float Game::direction(const sf::Vector2f& ver) {
	if (ver.x < 0 || ver.y < 0 || ver.x > BOARD_SIZE.x || ver.y > BOARD_SIZE.y)
		return -1.f;

	const float RADIUS = 1000;
	auto intersection = m_objectsOnBoard.colliding(ver, RADIUS);
	//auto myRad = m_data.find(m_id)->second.getRadius();
	float sum = 0;
	for (auto it = intersection.begin(); it != intersection.end(); ++it) {
		//if (!dynamic_cast<Bomb>(m_objectsOnBoard.find(it))
		//sum += (FOOD_RADIUS*(float(rand()%3)+1.f));
		//else if (isIntersect(m_data.find(*it)->second.getId()))//check there are no bombs
		sum += FOOD_RADIUS;
		//	sum = -100000.f;
	}
	return sum;
}
//====================================================================================
//===========================      RECEIVE DATA      =================================
//====================================================================================
//����� ��� �� ����
bool Game::receiveChanges(const Images &images)
{
	sf::Packet packet;
	m_socket.receive(packet);

	while (!packet.endOfPacket())
	{
		std::pair<Uint32, sf::Vector2f> temp;
		packet >> temp;
		std::vector<Uint32> del;

		if (temp.first >= 1000 && temp.first <= 10000) // ���� �� ����� �����
			m_objectsOnBoard.insert(temp);

		else if (temp.first >= 200 && temp.first <= 300)// ����
		{
			if (temp.first == m_me->getId())// ����� ���
				continue;
			if (m_players.find(temp.first) != m_players.end())// ����� �� ���� (���� ����..)�
			{
				m_players[temp.first]->setPosition(temp.second);
				m_players[temp.first]->setCenter(m_players[temp.first]->getPosition() + Vector2f{ m_players[temp.first]->getRadius(),m_players[temp.first]->getRadius() });
				if (!m_players[temp.first]->collision(del, m_objectsOnBoard, m_players, m_me.get()))
					return false; //�� ����� ��� ����
			}
			else // ���� ���
			{
				Uint32 image;
				packet >> image;
				m_players.emplace(temp.first, std::make_unique<OtherPlayers>(temp.first, images[image], NEW_PLAYER, temp.second));
			}
		}
	}

	return true;
}

//====================================================================================
//===========================          PRINT         =================================
//====================================================================================
void Game::setView(sf::RenderWindow &w) const
{
	sf::View view;
	view.reset(sf::FloatRect{ 0,0,float(SCREEN_WIDTH),float(SCREEN_HEIGHT) });
	sf::Vector2f pos{ float(SCREEN_WIDTH) / 2 , float(SCREEN_HEIGHT) / 2 };

	if (m_me->getCenter().x > SCREEN_WIDTH / 2)
		if (BOARD_SIZE.x - m_me->getCenter().x < SCREEN_WIDTH / 2)
			pos.x = BOARD_SIZE.x - SCREEN_WIDTH / 2;
		else
			pos.x = m_me->getCenter().x;

	if (m_me->getCenter().y > SCREEN_HEIGHT / 2)
		if (BOARD_SIZE.y - m_me->getCenter().y < SCREEN_HEIGHT / 2)
			pos.y = BOARD_SIZE.y - SCREEN_HEIGHT / 2;
		else
			pos.y = m_me->getCenter().y;

	view.setCenter(pos);
	w.setView(view);
}
//--------------------------------------------------------------------------
void Game::draw(sf::RenderWindow &w) const
{
	setView(w);

	w.clear();
	w.draw(m_background);

	for (auto &x : m_objectsOnBoard)
		w.draw(*x.second.get());

	for (auto &x : m_players)
		w.draw(*x.second.get());

	w.draw(*m_me.get());

	w.display();

}


//*************************************************************************************
//****************************    PLAYER FUNCTION   ***********************************
//*************************************************************************************
// ����� ��� �� ����
bool Player::collision(std::vector<Uint32> &deleted, Maps &objectsOnBoard, std::unordered_map<Uint32, std::unique_ptr<OtherPlayers>>& players, Player *me)
{
	checkFoodAndBomb(deleted, objectsOnBoard);
	return checkPlayers(deleted, players, me);
}
//--------------------------------------------------------------------------
bool Player::checkPlayers(std::vector<Uint32> &deleted, std::unordered_map<Uint32, std::unique_ptr<OtherPlayers>>& players, Player *me)
{
	bool temp = true;
	std::vector<Uint32> del;
	for (auto &player : players)
	{
		if (player.second->getId() == getId())
			continue;
		if (circlesCollide(player.second.get()))
			if (getRadius() > player.second->getRadius()) //�� ���� ������� ��� ����� ����� ����� ��
			{
				setScore(Uint32(player.second->getRadius()));
				newRadius(player.second.get());
				del.push_back(player.first);
				deleted.push_back(player.first);
			}
			else
				temp = (getId() == me->getId()) ? false : true; //�� ������ �� (�� ����� ���, ����� �����)�
	}

	if (getId() != me->getId()) //����� �� ���� ����� ��� ����� ���
	{
		if (circlesCollide(me))
			if (getRadius() > me->getRadius())
				temp = false;
			else
			{
				del.push_back(getId());
				me->setScore(Uint32(getRadius()));
				me->newRadius(this);
			}
	}

	for (auto pl : del)
		players.erase(pl);

	return temp;
}
//--------------------------------------------------------------------------
void Player::checkFoodAndBomb(std::vector<Uint32> &deleted, Maps &objectsOnBoard)
{
	std::set<Uint32> check = objectsOnBoard.colliding(getCenter(), getRadius());

	for (auto it : check) //����� �� ���� ������ ������ ������
		if (distance(getCenter(), objectsOnBoard[it]->getCenter()) <= getRadius() + objectsOnBoard[it]->getRadius())
		{
			newRadius(objectsOnBoard[it].get());
			objectsOnBoard.eraseFromData(it);
		}
}