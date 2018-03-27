// WZR_serwer.cpp : Defines the entry point for the console application.
//

#include <windows.h>
#include <time.h>
#include <stdio.h>
#include <vector>
#include "siec.h"
#include "kwaternion.h"
#include "obiekty.h"



enum typy_ramek { STAN_OBIEKTU, NEW_PLAYER, CLOSE_WINDOW };
Teren teren;

struct Ramka                                    // g³ówna struktura s³u¿¹ca do przesy³ania informacji
{
  int typ;
  long moment_wyslania;
  StanObiektu stan;                             
};

struct User {
	unsigned long addr; 
	int id; 
	StanObiektu stan;
};

unicast_net *uni_reciv;
unicast_net *uni_send;

std::vector<User*> klienci;
Ramka ramka;
StanObiektu stan;


bool isExist(unsigned long adres) {
	for (int i = 0; i < klienci.size(); i++) {
		if (klienci[i]->addr == adres && klienci[i]->id == ramka.stan.iID) {
			return true; 
		}
	}
	return false; 
}

int getIndex(unsigned long adres) {
	for (int i = 0; i < klienci.size(); i++) {
		if (klienci[i]->addr == adres && klienci[i]->id == ramka.stan.iID) {
			return i;
		}
	}
	return -1;
}

void addUser(unsigned long adres) {
	User* user = new User();
	user->addr = adres;
	user->stan = stan;
	user->id = ramka.stan.iID;
	klienci.push_back(user);
	printf("Connected %lu", adres);
}
void sendAllUsers(float x, float y, float z) {

	ramka.typ = 0;
	ramka.stan.wPol = teren.PunktMax(Wektor3(x, y, z)) + Wektor3(x, y, z)*(0.05 + 1.7 / 2 + 20);
	for (int i = 0; i < klienci.size(); i++) {
		printf("To %lu (%d)\n", klienci[i], klienci.size());
		uni_send->send((char*)&ramka, klienci[i]->addr, sizeof(Ramka));
	}
}
void sendAllUsers() {


	for (int i = 0; i < klienci.size(); i++) {
		printf("To %lu (%d)\n", klienci[i], klienci.size());
		uni_send->send((char*)&ramka, klienci[i]->addr, sizeof(Ramka));
	}
}
void findPointsAndSend()
{
	float maxOdl = 0;
	float odl = 0;
	float xmin = 1, ymin = 0, zmin = 0;
	User* klient;
	for (int j = 0; 10 > j; j++)
	{
		float x = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
		float y = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
		float z = sqrt(1.0 - x * x - y * y);
		if (rand() % 2 == 0)
		{
			x *= -1.0;
		}
		if (rand() % 2 == 0)
		{
			z *= -1.0;
		}
		if (rand() % 2 == 0)
		{
			y *= -1.0;
		}
		for(int i=0;klienci.size()>i;i++)
		{
			if (klienci[i]->id != ramka.stan.iID)
			{
				klient = klienci[i];
				odl += sqrt(pow(klienci[i]->stan.wPol.x - x, 2.0) + pow(klienci[i]->stan.wPol.y - y, 2.0) + pow(klienci[i]->stan.wPol.z - z, 2.0));
			}
		}
		if (odl > maxOdl)
		{
			maxOdl = odl;
			xmin = x;
			ymin = y;
			zmin = z;
		}
		odl = 0;

	}
	// obrót obiektu o k¹t 30 stopni wzglêdem osi y:
	sendAllUsers(xmin, ymin, zmin);
}


void removeUser(unsigned long adres) {
	int index = getIndex(adres); 
	if (index >= 0) {
		klienci.erase(klienci.begin()+index);
	}
}


int main()
{
	uni_reciv = new unicast_net(10002);
	uni_send = new unicast_net(10001);

	while (1) {
		int rozmiar;
		unsigned long adres;
		rozmiar = uni_reciv->reciv((char*)&ramka, &adres, sizeof(Ramka));
		switch (ramka.typ) {
		case typy_ramek::NEW_PLAYER:
			if (!isExist(adres)) {
				addUser(adres); 
			}
			printf("ramka NEW_PLAYER ");
			findPointsAndSend();
			break;
		case typy_ramek::STAN_OBIEKTU:
			/*printf("ramka STAN_OBIEKTU ");*/
			if (isExist(!adres)) {
				addUser(adres);
			}
			sendAllUsers(); 
			break; 
		case typy_ramek::CLOSE_WINDOW:
			removeUser(adres); 
			printf("ramka CLOSE_WINDOW ");
			break;
		}
	}

	return 0;
}


