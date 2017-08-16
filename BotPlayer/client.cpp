#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <iostream>
#include <string>

using namespace std;

#define BUFSIZE 4096
#define BLOCKED true
#define NONBLOCKED false

//commands
const char create[] = ".create\r\n";

const char start[] = "start\r\n";
const char who[] = ".who\r\n";
const char market[] = "market\r\n";
const char info[] = "info\r\n";
const char turn[] = "turn\r\n";
const char quit[] = "quit\r\n";
const char quit1[] = ".quit\r\n";

class player
{
	//player
	char *name, *game, *onlyname;
	int botswait;
	int raw, prod;
	int plants, autoplants, money;
	//market
	int minprice, maxprice;
	int rawm, prodm;
public:
	player(char** argv)
	{
		name = new char[strlen(argv[3])+2];
		game = new char[strlen(argv[5])+2];
		sprintf(name, "%s\r\n", argv[3]);
		onlyname = argv[3];//onlyname without "\r\n"
		
		botswait = 0;
		
		if (strstr(argv[4], "join") != NULL)
			sprintf(game, "%s\r\n", argv[5]);
		else
			botswait = atoi(argv[5]);
			
		prod = raw = plants = 2;
		autoplants = 0;
		money = 10000;
		minprice = 500;
		maxprice = 5500;
		rawm = prodm = 4;
	}
	
	player(const player& a)
	{		
		name = new char[strlen(a.name)];
		for (unsigned int i = 0; i < strlen(a.name); i++)
			name[i] = a.name[i];
			
		game = new char[strlen(a.game)];
		for (unsigned int i = 0; i < strlen(a.game); i++)
			game[i] = a.game[i];
			
		onlyname = a.onlyname;
		botswait = a.botswait;	
		prod = a.prod;
		raw = a.raw;
		plants = a.plants;
		autoplants = a.autoplants;
		money = a.money;
		minprice = a.minprice;
		maxprice = a.maxprice;
		rawm = a.rawm;
		prodm = a.prodm;
	}
	
	~player()
	{
		delete []name;
		delete []game;
	}
	
	int getraw() {return raw;}
	int getprod() {return prod;}
	int getplants() {return plants;}
	int getautoplants() {return autoplants;}
	int getmoney() {return money;}
	int getminprice() {return minprice;}
	int getmaxprice() {return maxprice;}
	int getrawm() {return rawm;}
	int getprodm() {return prodm;}
	int getbotswait() {return botswait;}
	char* getname() {return name;}
	char* getgame() {return game;}
	char* getonlyname() {return onlyname;}
	
	void print()
	{
		cout<<name<<"\n";
		cout<<"prod:"<<prod<<" raw:"<<raw<<" money:"<<money<<"\n";
		cout<<"plants:"<<plants<<" autoplants:"<<autoplants<<"\n";
		cout<<"Market\n";
		cout<<"prod:"<<prodm<<" raw:"<<rawm<<"\n";
		cout<<"minprice:"<<minprice<<" maxprice:"<<maxprice<<"\n";
	}
	
	//inserts player info
	void data(int r, int p, int mn, int pl, int apl)
	{
		raw = r;
		prod = p;
		money = mn;
		plants = pl;
		autoplants = apl;
	}
	
	//inserts market info
	void market(int r, int minp, int pr, int maxp)
	{
		rawm = r;
		minprice = minp;
		prodm = pr;
		maxprice = maxp;
	}
};

struct list 
{
	char *name;
	int sprice, samount, bprice, bamount;
	struct list *next;
	
	void clear(list *bot)	
	{
		while (bot != 0)
		{
			sprice = samount = bprice = bamount = 0;
			bot = bot->next;
		}
	}
	
	void print(list *bot)
	{
		cout<<"End of turn\n";
		while (bot != 0)
		{
			cout<<bot->name<<"\n";
			cout<<"Sold:"<<bot->samount<<" Price:"<<bot->sprice<<"\n";
			cout<<"Bought:"<<bot->bamount<<" Price:"<<bot->bprice<<"\n";
			bot = bot->next;
		}
	}
};

class buffer
{
	char *buf;
	int shift, shiftall, sock;
public:	
	buffer(int s)
	{
		buf = new char[BUFSIZE];
		shift = shiftall = 0;
		sock = s;
	}
	
	buffer(const buffer& a)
	{
		for(unsigned int i = 0; i < BUFSIZE; i++)
			buf[i] = a.buf[i];
		shift = a.shift;
		shiftall = a.shiftall;
		sock = a.sock;
	}
	
	~buffer() {delete []buf;}
	
	char* getbuf() {return buf;}
	
	void print() {cout<<buf;}
		
	void refresh() 
	{
		shift = shiftall = 0;
		buf[0] = '\0';
	}
	
	//read buf from socket (NON_BLOCKING)
	void readbuf(bool block)
	{
		refresh();
		timeval tv;
		fd_set readfds;
		//select's wait time will be -> 0(NON_BLOCKING)
		tv.tv_sec = 0;
		tv.tv_usec = 0;
		
		while (1)
		{	
			FD_ZERO(&readfds);
			FD_SET(sock, &readfds);
			
			if (block) //Blocked
			{
				if (select(sock + 1, &readfds, NULL, NULL, NULL) == -1)
					throw("Select error\n");
			}
			else //Nonblocked
			{
				if (select(sock + 1, &readfds, NULL, NULL, &tv) == -1)
					throw("Select error\n");
			}
			//Blocked only for 1 time, because torn message will be assembled and blocked after
			block = 0; 
					
			if (FD_ISSET(sock, &readfds))
			{
				shift = read(sock, buf + shiftall, BUFSIZE - shiftall);
				shiftall += shift;
				if (shift == -1)
					throw("Read error\n");
				if (shift == 0)
					throw("Connection lost\n");
			}
			else
			{
				buf[shiftall] = '\0';
				break;
			}
		}
	}
	
	//sends a command to server
	void action (const char* cmd)
	{
		int bytes;
		cout<<"BOTCOMMAND: "<<cmd;
		bytes = write(sock, cmd, strlen(cmd));
		if (bytes == -1)
			throw("Write error\n");
	}
	
	void join (player &bot)
	{
		sprintf(buf, ".join %s", bot.getgame());
		action(buf);
	}
	
	void hiddenaction (const char* cmd)
	{
		int bytes;
		bytes = write(sock, cmd, strlen(cmd));
		if (bytes == -1)
			throw("Write error\n");
	}
	
	//returns 1 if c exists in buf
	bool find (char c)
	{
		for(int i = 0; i < shiftall; i++)
			if (buf[i] == c)
				return 1;
		return 0;
	}
	
	//LET THE GAME BEGIN
	void epicstart(int line)
	{
		action(create);
		readbuf(BLOCKED);
		//print();
		while (line)
		{
			readbuf(BLOCKED);
			//print();
			//@+ JOIN Bot1
			if (find('@'))
				line--;
		}
		cout<<"Game is going to begin in 5 sec\n";
		sleep(2);
		cout<<"Three!\n";
		sleep(1);
		cout<<"Two!\n";
		sleep(1);
		cout<<"One...\n";
		sleep(1);
		cout<<"LET THE GAME BEGIN\n";
		action(start);
		readbuf(BLOCKED);
		//print();
	}
	
	//input name dynamically
	void nameinput(char* &name, char* pointer)
	{
		int i = 0, size = 32;
		//char *bigger;
		name = new char[size];
		while (pointer[i] != ' ')
		{
			name[i] = pointer[i];			
			name[i+1] = '\0';
			i++;
			/*if (i > (size - 1))
			{
				bigger = name;
				size = size*2;
				name = new char[size];
				for (int j = 0; j < i; j++)
					name[j] = bigger[j];
				delete []bigger;
			}*/
		} 
	}
	
	//inputs all names to list of players
	void playersinput(list *&first)
	{
		int names = 0;
		char *pointer;
		list *last = first;
		
		readbuf(NONBLOCKED);
		hiddenaction(who);
		readbuf(BLOCKED);
		pointer = buf;
		while (1)
		{
			names++;
			pointer = strstr(pointer, "% ");
			if (pointer == 0)
				break;
			else 
				pointer += 2;
		}		
		names = names - 2; //players in game
		cout<<"Players in game: "<<names<<"\n";
		
		pointer = buf;
		first = new list;
		last = first;
		while (names)
		{
			names--;
			pointer = strstr(pointer, "% ") + 2;
			nameinput(last->name, pointer);
			if(names != 0)
			{
				last->next = new list;
				last = last->next;
			}
			last->next = 0;
		}
		
		last = first;
		do {
			cout<<last->name<<"\n";
			last = last->next;
		} while (last != 0);
	}
	
	//inicialize bot
	void init(player &bot)
	{
		readbuf(BLOCKED);
		print();
		hiddenaction(bot.getname());
		readbuf(BLOCKED);
		if (buf[0] == '*')
			cout<<bot.getname();
		else 
			throw("\nRename me\n");
			
		//joining or creating the game
		if (bot.getbotswait() != 0)
			epicstart(bot.getbotswait());
		else
		{
			join(bot);
			while(1)//& START
			{
				readbuf(BLOCKED);
				//print();				
				if (find('&'))
					break;
			}
		}
	}
	
	//extracts int value from string of followung type
	//"       123 \0\n"
	int intextractor(char* pointer)
	{
		int x = 0;
		//skip spaces
		while (*pointer == ' ')
			pointer++;	
		//build number
		while ((*pointer != ' ') && (*pointer != '\n') && (*pointer != '\0'))
		{
			x = x*10 + (*pointer - '0');
			pointer++;
			if ((pointer - buf) >= BUFSIZE)
				break;
		}
		return x;
	}
	
	void endturn(list *&first)
	{
		char* pointer;
		list* last = first;
		first->clear(first);
		//# Trading results:
		//# --------              name     amount      price
		//& BOUGHT               ahmed          6        500
		//& SOLD                  oleg          2       5500
		//& SOLD                 boris          2       5500
		//& SOLD                 ahmed          2       5500
		pointer = buf;
		while (1)
		{
			last = first;
			pointer = strstr(pointer, "BOUGHT") ;
			if (pointer == 0)
				break;
			pointer = pointer + 7;
			
			while (*pointer == ' ')
				pointer++;
				
			while (strncmp(last->name, pointer, strlen(last->name)))
				last = last->next;
			pointer = pointer + strlen(last->name);
			last->bamount = intextractor(pointer);
			last->bprice = intextractor(pointer+12);
		}
		
		pointer = buf;		
		while (1)
		{
			last = first;
			pointer = strstr(pointer, "SOLD") ;
			if (pointer == 0)
				break;
			pointer = pointer + 7;
			
			while (*pointer == ' ')
				pointer++;
				
			while (strncmp(last->name, pointer, strlen(last->name)))
				last = last->next;
			pointer = pointer + strlen(last->name);
			last->samount = intextractor(pointer);
			last->sprice = intextractor(pointer+12);
		}
		first->print(first);
	}
	
	//resfeshes all info about market and player
	void refreshinfo (player &bot)
	{
		char* pointer;
		int a,b,c,d,f;
		//# ------        Raw  MinPrice      Prod  MaxPrice
		//& MARKET          4       500         4      5500
		//# ------
		hiddenaction(market);
		//prints everything untill "# --"
		while(1)
		{
			readbuf(BLOCKED);
			pointer = strstr(buf, "# --");
			if (((pointer - buf) > 0) && (pointer != 0))
			{
				*pointer = '\0';
				pointer++;
				//print();
			}
			if (pointer != 0)
				break;
			//print();
		}
		pointer = strstr(pointer, "MARKET") + 6;
		a = intextractor(pointer);
		b = intextractor(pointer+12);
		c = intextractor(pointer+22);
		d = intextractor(pointer+32);
		bot.market(a, b, c, d);
		
		//# -----             Name  Raw Prod    Money Plants AutoPlants
		//& INFO              john    2    2    10000    2    0
		//& INFO             boris    2    2    10000    2    0
		//# -----
		hiddenaction(info);
		//prints everything untill "# --"
		while(1)
		{
			readbuf(BLOCKED);
			pointer = strstr(buf, "# --");
			if (((pointer - buf) > 0) && (pointer != 0))
			{
				*pointer = '\0';
				pointer++;
				//print();
			}
			if (pointer != 0)
				break;
			//print();
		}
		//matches name
		pointer = strstr(pointer, bot.getonlyname()) + strlen(bot.getonlyname());		
		a = intextractor(pointer);
		b = intextractor(pointer+5);
		c = intextractor(pointer+10);
		d = intextractor(pointer+20);
		f = intextractor(pointer+25);
		bot.data(a, b, c, d, f);
	}
	
	void sell(player &bot)
	{
		int maxprice, prod;
		
		refreshinfo(bot);
		maxprice = bot.getmaxprice();
		prod = bot.getprod();
		if (prod != 0)
		{
			sprintf(buf,"sell %d %d\r\n", prod, maxprice);
			action(buf);
			readbuf(BLOCKED);
			//print();
		}
	}
	
	void buy(player &bot)
	{
		int minprice, raw, rawm, money;
		
		refreshinfo(bot);
		
		minprice = bot.getminprice();
		raw = bot.getraw();
		money = bot.getmoney();
		rawm = bot.getrawm();
				
		raw = money/minprice;		
		if (raw > rawm)
			raw = rawm;
		if (raw != 0)
		{
			sprintf(buf,"buy %d %d\r\n", raw, minprice);
			action(buf);
			readbuf(BLOCKED);
			//print();
		}
	}
	
	void prod(player &bot)
	{		
		int raw, prod;
		
		refreshinfo(bot);
		raw = bot.getraw();
		prod = 2;
		if ((raw > 0) && (prod > 0))
		{
			if (prod > raw)
				prod = raw;
			sprintf(buf,"prod %d\r\n", prod);
			action(buf);
			readbuf(BLOCKED);
			//print();
		}
	}
};

//The Game
void game(int sock, char** argv)
{
	player bot(argv);
	buffer buf(sock);
	list *players = 0;
	
	buf.init(bot);
	buf.playersinput(players);
	cout<<"\n";
	
	while(1)
	{
		buf.prod(bot);
		buf.sell(bot);
		buf.buy(bot);
		buf.action(turn);
		do
		{
			buf.readbuf(BLOCKED);
			//buf.print();
		} while (strstr(buf.getbuf(), "ENDTURN") == 0);
		buf.endturn(players);
		cout<<"\n";
		//sleep(1);
		if ((strstr(buf.getbuf(), "WIN") != NULL) ||
		(strstr(buf.getbuf(), "You are a bankrupt, sorry")))
		{
			if (strstr(buf.getbuf(), "You are a bankrupt, sorry"))
				cout<<bot.getonlyname()<<" is a bankrupt\n";
			else
				cout<<bot.getonlyname()<<" won the game\n";
			buf.action(quit);
			buf.action(quit1);
			break;
		}
		
	}
}

int main(int argc, char *argv[])
{
	try
	{
		int sock, port;
		struct sockaddr_in addr;
		
		port = atoi(argv[1]);
		
		sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (sock == -1)
			throw("Socket error\n");
		
		addr.sin_family = AF_INET;
		addr.sin_port = htons(port);
		addr.sin_addr.s_addr = htonl(atol(argv[2]));

		if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) == -1)
			throw("Can't connect to server\n");
		
		game(sock, argv);
		
		shutdown(sock, 2);
		close(sock);
	}
	catch (const char* s)
	{
		cout<<s;
	}
	return 0;
}
