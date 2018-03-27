/*********************************************************************
    Symulacja obiektów fizycznych ruchomych np. samochody, statki, roboty, itd. 
    + obs³uga obiektów statycznych np. teren.
**********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <windows.h>
#include <gl\gl.h>
#include <gl\glu.h>
#include "obiekty.h"
#include "grafika.h"
/*#include "wektor.h"*/

//#include "wektor.h"
extern FILE *f;
extern Teren teren;
extern int iLiczbaCudzychOb;
extern bool czy_rysowac_ID;




//*************************
//   Obiekty nieruchome
//*************************
Teren::Teren()
{
   char nazwa[128],nazwa_pliku[128],nazwa_pliku2[128];
   strcpy(nazwa,"planeta5");
   int wynik = tworz_z_obj(&siatka,nazwa_pliku);      // wczytanie siatki tójk¹tów z formatu *.obj (Wavefront)
   if (wynik == 0)
     int wynik = tworz_z_obj(&siatka,nazwa_pliku2);      // wczytanie siatki tójk¹tów z formatu *.obj (Wavefront)

   float skala = 2000.0;         

   for (long i=0;i<siatka.liczba_wezlow;i++)              // przeskalowanie siatki
     siatka.wezly[i].wPol = siatka.wezly[i].wPol*skala;


    

   for (long i=0;i<siatka.liczba_scian;i++)               // wyznaczenie normalnych do poszcz. œcian i wierzcho³ków
   {
     Wektor3 A = siatka.wezly[siatka.sciany[i].ind1].wPol, B = siatka.wezly[siatka.sciany[i].ind2].wPol,
       C = siatka.wezly[siatka.sciany[i].ind3].wPol;

     siatka.sciany[i].normalna = normalna(A,B,C);         // normalna do trojkata ABC
     siatka.wezly[siatka.sciany[i].ind1].normalna = siatka.wezly[siatka.sciany[i].ind2].normalna =
       siatka.wezly[siatka.sciany[i].ind3].normalna = siatka.sciany[i].normalna;  // normalna do wyg³adzania
   }

   // Wyznaczenie œrodka ciê¿koœci -> œrodka przyci¹gania grawitacyjnego
   float x_min=1e10,x_max=-1e10,y_min=1e10,y_max=-1e10,z_min=1e10,z_max=-1e10;
   for (long i=0;i<siatka.liczba_scian;i++)
   {
     Wektor3 P = siatka.wezly[siatka.sciany[i].ind1].wPol;
     if (x_min > P.x) x_min = P.x; 
     if (x_max < P.x) x_max = P.x;
     if (y_min > P.y) y_min = P.y; 
     if (y_max < P.y) y_max = P.y;
     if (z_min > P.z) z_min = P.z; 
     if (z_max < P.z) z_max = P.z;
   }
   srodek = Wektor3((x_max+x_min)/2, (y_max+y_min)/2, (z_max+z_min)/2);

   // Wyznaczenie œredniego promienia kuli:
   promien_sredni = 0;
   for (long i=0;i<siatka.liczba_wezlow;i++) 
     promien_sredni += (siatka.wezly[i].wPol - srodek).dlugosc()/siatka.liczba_wezlow;


   rozmiar_sektora = (x_max-x_min)/20;   // rozmiar sektora szeœciennego 

   tab_sekt = new TabSektorow(10000);

   // Umieszczenie œcian w sektorach, a sektorów w tablicy rozproszonej, by szybko je wyszukiwaæ:
   for (long i=0;i<siatka.liczba_scian;i++)               
   {
     float x_min=1e10,x_max=-1e10,y_min=1e10,y_max=-1e10,z_min=1e10,z_max=-1e10;  // granice przedzia³ów, w których mieœci siê œciana
     long e[] = {siatka.sciany[i].ind1, siatka.sciany[i].ind2, siatka.sciany[i].ind3};
     Wektor3 N = siatka.sciany[i].normalna, P = siatka.wezly[e[0]].wPol, 
             E0 = siatka.wezly[e[0]].wPol, E1 = siatka.wezly[e[1]].wPol, E2 = siatka.wezly[e[2]].wPol;

     for (long j=0;j<3;j++)      // po wierzcho³kach trójk¹ta
     {
       Wektor3 P = siatka.wezly[e[j]].wPol;
       if (x_min > P.x) x_min = P.x; 
       if (x_max < P.x) x_max = P.x;
       if (y_min > P.y) y_min = P.y; 
       if (y_max < P.y) y_max = P.y;
       if (z_min > P.z) z_min = P.z; 
       if (z_max < P.z) z_max = P.z;
     }
     // wyznaczenie indeksów sektorów dla granic przedzia³ów:
     long ind_x_min = (long)floor(x_min/rozmiar_sektora), ind_x_max = (long)floor(x_max/rozmiar_sektora),
       ind_y_min = (long)floor(y_min/rozmiar_sektora), ind_y_max = (long)floor(y_max/rozmiar_sektora),
       ind_z_min = (long)floor(z_min/rozmiar_sektora), ind_z_max = (long)floor(z_max/rozmiar_sektora);

     //fprintf(f,"i=%d, ind_x_min = %d, ind_x_max = %d\n",i,ind_x_min,ind_x_max);

     // skanowanie po wszystkich sektorach prostopad³oœcianu, w którym znajduje siê trójk¹t by sprawdziæ czy jakiœ
     // fragment trójk¹ta znajduje siê w sektorze. Jeœli tak, to sektor umieszczany jest w tablicy rozproszonej:
     for (long x = ind_x_min;x<=ind_x_max;x++)
       for (long y = ind_y_min;y<=ind_y_max;y++)
         for (long z = ind_z_min;z<=ind_z_max;z++)
         {
           bool czy_zawiera = 0;
           float x1 = (float)x*rozmiar_sektora, x2 = (float)(x+1)*rozmiar_sektora,  // granice sektora
             y1 = (float)y*rozmiar_sektora, y2 = (float)(y+1)*rozmiar_sektora,
             z1 = (float)z*rozmiar_sektora, z2 = (float)(z+1)*rozmiar_sektora;

           // sprawdzenie czy wewn¹trz szeœcianu sektora nie ma któregoœ z 3 wierzcho³ków: 
           for (long j=0;j<3;j++)      // po wierzcho³kach trójk¹ta
           {
             Wektor3 P = siatka.wezly[e[j]].wPol;
             if ((P.x >= x1)&&(P.x <= x2)&&(P.y >= y1)&&(P.y <= y2)&&(P.z >= z1)&&(P.z <= z2))
             {
               czy_zawiera = 1;
               break;
             }
           }

           // sprawdzenie, czy któryœ z 12 boków szeœcianu nie przecina œciany:
           if (czy_zawiera == 0)
           {

             // zbiór par wierzcho³ków definiuj¹cych boki:
             Wektor3 AB[][2] = {{Wektor3(x1,y1,z1), Wektor3(x2,y1,z1)}, {Wektor3(x1,y2,z1), Wektor3(x2,y2,z1)},  // w kier. osi x
                                {Wektor3(x1,y1,z2), Wektor3(x2,y1,z2)}, {Wektor3(x1,y2,z2), Wektor3(x2,y2,z2)},
                                {Wektor3(x1,y1,z1), Wektor3(x1,y2,z1)}, {Wektor3(x2,y1,z1), Wektor3(x2,y2,z1)},  // w kier. osi y
                                {Wektor3(x1,y1,z2), Wektor3(x1,y2,z2)}, {Wektor3(x2,y1,z2), Wektor3(x2,y2,z2)},
                                {Wektor3(x1,y1,z1), Wektor3(x1,y1,z2)}, {Wektor3(x2,y1,z1), Wektor3(x2,y1,z2)},  // w kier. osi z
                                {Wektor3(x1,y2,z1), Wektor3(x1,y2,z2)}, {Wektor3(x2,y2,z1), Wektor3(x2,y2,z2)}};

             for (long j=0;j<12;j++)
             {
               Wektor3 A = AB[j][0], B = AB[j][1];
               Wektor3 X = punkt_przec_prostej_z_plaszcz(A,B,N,P);
               if (czy_w_trojkacie(E0,E1,E2, X)&&(((X-A)^(X-B)) <= 0)){
                 czy_zawiera = 1;
                 break;
               }
             }
           }

           // sprawdzenie, czy któryœ z 3 boków trójk¹ta nie przecina którejœ z 6 œcian szeœcianu:
           if (czy_zawiera == 0)
           {
             Wektor3 AB[][2] = {{E0, E1}, {E1, E2}, {E2, E0}};
             for (long j=0;j<3;j++) // po bokach trójk¹ta
             {
               Wektor3 A = AB[j][0], B = AB[j][1];
               Wektor3 X = punkt_przec_prostej_z_plaszcz(A,B,Wektor3(1,0,0),Wektor3(x1,y1,z1));     // p³aszczyzna x = x1
               if ((X.y >= y1)&&(X.y <= y2)&&(X.z >= z1)&&(X.z <= z2)) { czy_zawiera = 1; break; }
               X = punkt_przec_prostej_z_plaszcz(A,B,Wektor3(1,0,0),Wektor3(x2,y1,z1));             // p³aszczyzna x = x2
               if ((X.y >= y1)&&(X.y <= y2)&&(X.z >= z1)&&(X.z <= z2)) { czy_zawiera = 1; break; }
               X = punkt_przec_prostej_z_plaszcz(A,B,Wektor3(0,1,0),Wektor3(x1,y1,z1));             // p³aszczyzna y = y1
               if ((X.x >= x1)&&(X.x <= x2)&&(X.z >= z1)&&(X.z <= z2)) { czy_zawiera = 1; break; }
               X = punkt_przec_prostej_z_plaszcz(A,B,Wektor3(0,1,0),Wektor3(x1,y2,z1));             // p³aszczyzna y = y2
               if ((X.x >= x1)&&(X.x <= x2)&&(X.z >= z1)&&(X.z <= z2)) { czy_zawiera = 1; break; }
               X = punkt_przec_prostej_z_plaszcz(A,B,Wektor3(0,0,1),Wektor3(x1,y1,z1));             // p³aszczyzna z = z1
               if ((X.x >= x1)&&(X.x <= x2)&&(X.y >= y1)&&(X.y <= y2)) { czy_zawiera = 1; break; }
               X = punkt_przec_prostej_z_plaszcz(A,B,Wektor3(0,0,1),Wektor3(x1,y1,z2));             // p³aszczyzna z = z2
               if ((X.x >= x1)&&(X.x <= x2)&&(X.y >= y1)&&(X.y <= y2)) { czy_zawiera = 1; break; }
             }

           }

           if (czy_zawiera)
           {
             SektorObiektu *sektor = tab_sekt->znajdz(x,y,z);
             if (sektor == NULL)
             {
               SektorObiektu sek;
               sek.liczba_scian = 1;
               sek.rozmiar_pamieci = 1;
               sek.sciany = new Sciana*[1];
               sek.sciany[0] = &siatka.sciany[i];
               sek.x = x; sek.y = y; sek.z = z;
               tab_sekt->wstaw(sek);
             }
             else
             {              
               if (sektor->liczba_scian >= sektor->rozmiar_pamieci)
               {
                 long nowy_rozmiar = sektor->rozmiar_pamieci*2;
                 Sciana **sciany2 = new Sciana*[nowy_rozmiar];
                 for (long j=0;j<sektor->liczba_scian;j++) sciany2[j] = sektor->sciany[j];
                 delete sektor->sciany;
                 sektor->sciany = sciany2;
                 sektor->rozmiar_pamieci = nowy_rozmiar;
               }
               sektor->sciany[sektor->liczba_scian] = &siatka.sciany[i];
               sektor->liczba_scian ++;
             }
           }


         } // 3x for po sektorach prostopad³oœcianu 

   } // for po œcianach

   long liczba_sektorow = 0, liczba_wsk_scian = 0;
   for (long i=0;i<tab_sekt->liczba_komorek;i++)
   {
     //fprintf(f,"komorka %d zaweira %d sektorow\n",i,tab_sekt->komorki[i].liczba_sektorow);
     liczba_sektorow += tab_sekt->komorki[i].liczba_sektorow;
     for (long j=0;j<tab_sekt->komorki[i].liczba_sektorow;j++)
     {
       //fprintf(f,"  sektor %d (%d, %d, %d) zawiera %d scian\n",j,tab_sekt->komorki[i].sektory[j].x,
       //  tab_sekt->komorki[i].sektory[j].y, tab_sekt->komorki[i].sektory[j].z,
       //  tab_sekt->komorki[i].sektory[j].liczba_scian);
       liczba_wsk_scian += tab_sekt->komorki[i].sektory[j].liczba_scian;
     }
   }
   //fprintf(f,"laczna liczba sektorow = %d, wskaznikow scian = %d\n",liczba_sektorow, liczba_wsk_scian);

   //rozmiar_sektora = 10;
   //SektorObiektu **s = NULL;
   //long liczba_s = SektoryWkierunku(&s, Wektor3(35,-5,0), Wektor3(-15,35,0));
   //long liczba_s = SektoryWkierunku(&s, Wektor3(33,55,0),Wektor3(5,5,0) );
}

Teren::~Teren()
{            
  delete siatka.wezly; delete siatka.sciany;           
}

// Wyznaczanie sektorów przecinanych przez wektor o podanym punkcie startowym i koñcowym
// zwraca liczbê znalezionych sektorów
long Teren::SektoryWkierunku(SektorObiektu ***wsk_sekt, Wektor3 punkt_startowy, Wektor3 punkt_koncowy)
{
  long liczba_sektorow = 0;
  long liczba_max = 10;
  SektorObiektu **s = new SektorObiektu*[liczba_max];

  // wyznaczenie wspó³rzêdnych pocz¹tku sektora startowego i koñcowego:
  long x_start = (long)floor(punkt_startowy.x/rozmiar_sektora), y_start = (long)floor(punkt_startowy.y/rozmiar_sektora),
       z_start = (long)floor(punkt_startowy.z/rozmiar_sektora);
  long x_koniec = (long)floor(punkt_koncowy.x/rozmiar_sektora), y_koniec = (long)floor(punkt_koncowy.y/rozmiar_sektora),
    z_koniec = (long)floor(punkt_koncowy.z/rozmiar_sektora);
   
  float krok = (x_start < x_koniec ? 1 : -1); 
  for (long x=x_start;x!=x_koniec;x+=krok)
  {
    float xf = (float)(x+krok+(krok==-1))*rozmiar_sektora;   // wsp. x punktu przeciecia wektora z górn¹ p³aszczyzn¹ sektora
  	// wyznaczenie punktu przeciêcia wektora z kolejn¹ p³aszczyzn¹ w kierunku osi x (dla indeksu x):
	  Wektor3 P = punkt_przec_prostej_z_plaszcz(punkt_startowy, punkt_koncowy, Wektor3(1,0,0), Wektor3(xf,0,0));
	  // wyznaczenie indeksów sektora, którego górna p³aszczyzna jest przecinana: 
	  long y = (long)floor(P.y/rozmiar_sektora), z = (long)floor(P.z/rozmiar_sektora);
	  // dodanie sektora do listy:
    SektorObiektu *sektor = tab_sekt->znajdz(x,y,z);
    //fprintf(f,"dla x = %d, sektor (%d, %d, %d)\n",x,x,y,z);
	  if (sektor){
		  s[liczba_sektorow] = sektor;
		  liczba_sektorow++;
	  }
	  if (liczba_max <= liczba_sektorow)
	  {
			long nowa_liczba_max = liczba_max*2;
			SektorObiektu **s2 = new SektorObiektu*[nowa_liczba_max];
			for (long i=0;i<liczba_sektorow;i++) s2[i] = s[i];
			delete s; s = s2; liczba_max = nowa_liczba_max;
	  }
  }
  krok = (y_start < y_koniec ? 1 : -1); 
	for (long y=y_start;y!=y_koniec;y+=krok)
  {
    float yf = (float)(y+krok+(krok==-1))*rozmiar_sektora;   // wsp. x punktu przeciecia wektora z górn¹ p³aszczyzn¹ sektora
  	// wyznaczenie punktu przeciêcia wektora z kolejn¹ p³aszczyzn¹ w kierunku osi x (dla indeksu x):
	  Wektor3 P = punkt_przec_prostej_z_plaszcz(punkt_startowy, punkt_koncowy, Wektor3(0,1,0), Wektor3(0,yf,0));
	  // wyznaczenie indeksów sektora, którego górna p³aszczyzna jest przecinana: 
	  long x = (long)floor(P.x/rozmiar_sektora), z = (long)floor(P.z/rozmiar_sektora);
	  // dodanie sektora do listy:

    SektorObiektu *sektor = tab_sekt->znajdz(x,y,z);
    //fprintf(f,"dla y = %d, sektor (%d, %d, %d)\n",y,x,y,z);
	  if (sektor){
		  s[liczba_sektorow] = sektor;
		  liczba_sektorow++;
	  }
	  if (liczba_max <= liczba_sektorow)
	  {
			long nowa_liczba_max = liczba_max*2;
			SektorObiektu **s2 = new SektorObiektu*[nowa_liczba_max];
			for (long i=0;i<liczba_sektorow;i++) s2[i] = s[i];
			delete s; s = s2; liczba_max = nowa_liczba_max;
	  }
  }
	krok = (z_start < z_koniec ? 1 : -1); 
	for (long z=z_start;z!=z_koniec;z+=krok)
  {
    float zf = (float)(z+krok+(krok==-1))*rozmiar_sektora;   // wsp. x punktu przeciecia wektora z górn¹ p³aszczyzn¹ sektora
  	// wyznaczenie punktu przeciêcia wektora z kolejn¹ p³aszczyzn¹ w kierunku osi x (dla indeksu x):
	  Wektor3 P = punkt_przec_prostej_z_plaszcz(punkt_startowy, punkt_koncowy, Wektor3(0,0,1), Wektor3(0,0,zf));
	  // wyznaczenie indeksów sektora, którego górna p³aszczyzna jest przecinana: 
	  long x = (long)floor(P.x/rozmiar_sektora), y = (long)floor(P.y/rozmiar_sektora);
	  // dodanie sektora do listy:
    SektorObiektu *sektor = tab_sekt->znajdz(x,y,z);
    //fprintf(f,"dla z = %d, sektor (%d, %d, %d)\n",z,x,y,z);
	  if (sektor){
		  s[liczba_sektorow] = sektor;
		  liczba_sektorow++;
	  }
	  if (liczba_max <= liczba_sektorow)
	  {
			long nowa_liczba_max = liczba_max*2;
			SektorObiektu **s2 = new SektorObiektu*[nowa_liczba_max];
			for (long i=0;i<liczba_sektorow;i++) s2[i] = s[i];
			delete s; s = s2; liczba_max = nowa_liczba_max;
	  }
  }
	SektorObiektu *sektor = tab_sekt->znajdz(x_koniec,y_koniec,z_koniec);
  //fprintf(f,"punkt koncowy, sektor (%d, %d, %d)\n",x_koniec,y_koniec,z_koniec);
	if (sektor){
		  s[liczba_sektorow] = sektor;
		  liczba_sektorow++;
	}

  (*wsk_sekt) = s;

  return liczba_sektorow;
}

Wektor3 Teren::PunktSpadku(Wektor3 P)     // okreœlanie wysokoœci dla punktu P (odleg³oœci od najbli¿szej p³aszczyzny w kierunku grawitacji)
{                                         // 
  float odl_min = 1e10;
  Wektor3 Pp_min = this->srodek;          // œrodek bry³y
  long ind_min = 0;                      
  SektorObiektu **s = NULL;               // tablica wskaŸników sektorów                                   
  long liczba_s = SektoryWkierunku(&s, P, this->srodek); // wyszukanie sektorów przecinanych przez wektor P_srodek, w których znajduj¹ siê œciany 
  for (long j=0;j<liczba_s;j++)           // po niepustych sektorach
  {
    for (long i=0;i<s[j]->liczba_scian;i++)  // po œcianach z sektora j-tego
    {
      Wektor3 A = siatka.wezly[s[j]->sciany[i]->ind1].wPol, B = siatka.wezly[s[j]->sciany[i]->ind2].wPol,
         C = siatka.wezly[s[j]->sciany[i]->ind3].wPol;  // po³o¿enia wierzcho³ków trójk¹ta œciany
      Wektor3 Pp = punkt_przec_prostej_z_plaszcz(P, this->srodek, s[j]->sciany[i]->normalna,A); // punkt przeciêcia wektora P_srodek z p³aszczyzn¹ j-tej œciany
      bool czy_w_tr = czy_w_trojkacie(A, B, C, Pp);     // sprawdzenie czy punkt przeciêcia znajduje siê wewn¹trz trójk¹ta œciany
      if (czy_w_tr)                                     // jeœli punkt przeciêcia jest wewn¹trz trójk¹ta œciany
      {                                                 // to sprawdzenie, czy punkt ten le¿y najbli¿ej od punktu P
        float odl_od_plasz = (P-Pp).dlugosc();
        float odl_od_srodka = (P-srodek).dlugosc();
        if ((odl_od_srodka >= odl_od_plasz)&&(odl_min > odl_od_plasz))
        {
          odl_min = odl_od_plasz;
          ind_min = i;
          Pp_min = Pp;
        }
      }
    } // for po œcianach
  } // for po sektorach
  delete s;

  return Pp_min;    
}

Wektor3 Teren::PunktMax(Wektor3 v)     // okreœlanie punktu le¿¹cego jak najdalej od œrodka sfery w kierunku v
{
  float odl_max = 0;
  Wektor3 Pp_max = this->srodek;
  long ind_max = 0;
  for (long i=0;i<siatka.liczba_scian;i++)
  {
    Wektor3 A = siatka.wezly[siatka.sciany[i].ind1].wPol, B = siatka.wezly[siatka.sciany[i].ind2].wPol,
       C = siatka.wezly[siatka.sciany[i].ind3].wPol;
    Wektor3 Pp = punkt_przec_prostej_z_plaszcz(this->srodek, this->srodek + v,siatka.sciany[i].normalna,A);
    bool czy_w_tr = czy_w_trojkacie(A, B, C, Pp);
      

    if ((czy_w_tr)&&(((Pp-srodek)^v) > 0)) // czy punkt w trójk¹cie i w kierunku v 
    {
      
      float odl_od_srodka = (Pp-srodek).dlugosc();
      if (odl_max < odl_od_srodka)
      {
        odl_max = odl_od_srodka;
        ind_max = i;
        Pp_max = Pp;
      }
    }

  }

  return Pp_max;    
}



   
TabSektorow::TabSektorow(long liczba_kom)
{
  liczba_komorek = liczba_kom;
  komorki = new KomorkaTablicy[liczba_komorek];
  for (long i=0;i<liczba_komorek;i++)
  {
    komorki[i].liczba_sektorow = 0;
    komorki[i].rozmiar_pamieci = 0;
    komorki[i].sektory = NULL;  
  }
}

TabSektorow::~TabSektorow()
{
  for (long i=0;i<liczba_komorek;i++) 
    delete komorki[i].sektory;
  delete komorki;
}
      
long TabSektorow::wyznacz_klucz(long x, long y, long z)                  // wyznaczanie indeksu komórki 
{
  long kl = (long)fabs((cos(x*1.0)*cos(y*1.1)*cos(z*1.2)+cos(x*1.0)+cos(y*1.0)+cos(z*1.0))*liczba_komorek*10) % liczba_komorek;
  return kl;
}

SektorObiektu *TabSektorow::znajdz(long x, long y, long z)       // znajdowanie sektora (zwraca NULL jeœli nie znaleziono)
{
  long klucz = wyznacz_klucz(x,y,z);
  SektorObiektu *sektor = NULL;
  for (long i=0;i<komorki[klucz].liczba_sektorow;i++)
  {
    SektorObiektu *s = &komorki[klucz].sektory[i];
    if ((s->x == x)&&(s->y == y)&&(s->z == z)) {
      sektor = s;
      break;
    }
  }
  return sektor;
}

SektorObiektu *TabSektorow::wstaw(SektorObiektu s)      // wstawianie sektora do tablicy
{
  long x = s.x, y = s.y, z = s.z;
  long klucz = wyznacz_klucz(x,y,z);
  long liczba_sekt = komorki[klucz].liczba_sektorow;
  if (liczba_sekt >= komorki[klucz].rozmiar_pamieci)     // jesli brak pamieci, to nale¿y j¹ powiêkszyæ 
  {
    long nowy_rozmiar = (komorki[klucz].rozmiar_pamieci == 0 ? 1: komorki[klucz].rozmiar_pamieci*2);
    SektorObiektu *sektory2 = new SektorObiektu[nowy_rozmiar];
    for (long i=0;i<komorki[klucz].liczba_sektorow;i++) sektory2[i] = komorki[klucz].sektory[i];
    delete komorki[klucz].sektory;
    komorki[klucz].sektory = sektory2;
    komorki[klucz].rozmiar_pamieci = nowy_rozmiar;
  }
  komorki[klucz].sektory[liczba_sekt] = s; 
  komorki[klucz].liczba_sektorow++;

  return &komorki[klucz].sektory[liczba_sekt];           // zwraca wskaŸnik do nowoutworzonego sektora
}

/*
    Wczytywanie eksportowanego z Blendera formatu obj, w ktorym zapisywane sa wierzcholki i sciany
    Zwraca 1 w razie powodzenia, a 0 gdy nie da siê odczytaæ pliku
*/
int tworz_z_obj(SiatkaObiektu *si,char nazwa_pliku[])
{
  FILE *f = fopen(nazwa_pliku,"r");

  if (f == NULL)
    return 0; 

  char lan[1024],napis[128],s1[128],s2[128],s3[128],s4[128];

  long liczba_max_wezlow = 100;
  long liczba_max_scian = 100;

  si->wezly = new Wezel[liczba_max_wezlow];
  si->liczba_wezlow = 0;
  si->sciany = new Sciana[liczba_max_scian];
  si->liczba_scian = 0;

  long lw = si->liczba_wezlow, ls = si->liczba_scian;

  while (fgets(lan,1024,f))
  {
    sscanf(lan,"%s",&napis);

    if (strcmp(napis,"v") == 0)     // znaleziono dane wierzcholka -> tworze nowa kule
    {
      sscanf(lan,"%s %s %s %s",&napis,&s1,&s2,&s3);
      si->wezly[lw].wPol = Wektor3(atof(s1),atof(s2),atof(s3));

      lw++;  si->liczba_wezlow = lw;

      if (si->liczba_wezlow >= liczba_max_wezlow)  // jesli pamiec na kule wyczerpala sie
      {
        liczba_max_wezlow *= 2;
        Wezel *wezly_pom = si->wezly;
        si->wezly = new Wezel[liczba_max_wezlow];
        for (long i=0;i<si->liczba_wezlow;i++) si->wezly[i] = wezly_pom[i];
        delete wezly_pom;
      }
    }

    if (strcmp(napis,"f") == 0)  // znaleziono trojkat (lub czworokat) -> tworze nowa scianke 
    {
      int wynik = sscanf(lan,"%s %s %s %s %s",&napis,&s1,&s2,&s3,&s4);
      int i1 = atoi(s1)-1, i2 = atoi(s2)-1, i3 = atoi(s3)-1;  // w pliku *.obj wezly indeksowane sa od 1, stad trzeba odjac 1 by przejsc na ind. od 0
      si->sciany[ls].ind1 = i1; si->sciany[ls].ind2 = i2; si->sciany[ls].ind3 = i3;
      ls++; si->liczba_scian = ls;

      if (wynik == 5)  // jesli siatka czworokatow a nie trojkatow
      {
        int i4 = atoi(s4)-1;
        si->sciany[ls].ind1 = i3; si->sciany[ls].ind2 = i4; si->sciany[ls].ind3 = i1;
        ls++; si->liczba_scian = ls;

      }
      if (si->liczba_scian >= liczba_max_scian)  // jesli pamiec na kule wyczerpala sie
      {
        liczba_max_scian *= 2;
        Sciana *sciany_pom = si->sciany;
        si->sciany = new Sciana[liczba_max_scian];
        for (long i=0;i<si->liczba_scian;i++) si->sciany[i] = sciany_pom[i];
        delete sciany_pom;
      }

    }

  } // while po wierszach pliku

  fclose(f);

  return 1;
}