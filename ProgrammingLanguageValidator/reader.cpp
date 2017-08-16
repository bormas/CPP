#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <iostream>

#define BUFSIZE 1024

using namespace std;

enum stateenum
{
	home, //initial state
	number, //reading number
	identifier, //reading identifier
	keyword, //reading keyword
	str, //reading string
	assignment //:=	
};

enum typeenum
{
	no_type,
	number_type,
	variable_type,
	label_type,
	string_type,
	if_type,
	then_type,
	else_type,
	while_type,
	goto_type,
	print_type,
	assignment_type	
};

const char* type_names[12] = 
{"", "number", "variable", "label", "string", "if", 
"then", "else", "while", "goto", "print", "assignment"};

#if 0
//list of variables
struct list
{
	char* name;
	int value;	
	list* next;
};

//***************list functions***************
void init_list(list *&p)
{
	p = new list;
	p->name = 0;
	p->next = 0;
}

//adds new elem to list
void add_list(list *&p, const char* str, int x)
{
	p->name = new char[sizeof(str)];
	strcpy(p->name, str);
	p->value = x;
	p->next = new list;
	p = p->next;
	p->name = 0;
	p->next = 0;
}

//clears list
void clear_list(list *&p)
{
	list *a;
	while (p != 0)
	{
		delete p->name;
		a = p;
		p = p->next;
		delete a;
	}
}

//find list element with name "str"
bool find_list(list *p, const char* str)
{
	while (p != 0)
	{
		if (strcmp(p->name, str) == 0)
			return true;
		p = p->next;
	}
	return false;
}

//insert value in list of variables with name "str" 
void insert_list(list *p, const char* str, int x)
{
	while (p != 0)
	{
		if (strcmp(p->name, str) == 0)
		{
			p->value = x;
			break;
		}
		p = p->next;
	}
}

//get value of variables with name "str"
int getvalue_list(list *p, const char* str)
{
	while (p != 0)
	{
		if (strcmp(p->name, str) == 0)
			return p->value;
		p = p->next;
	}
	throw("There's no variable of such name");
}

//***************end of list functions***************
#endif

const char symbols[] = " \t\n+-*/%<>=,;";

class buffer
{
	char *buf; //buffer
	int shift; //buf index
	int line; //line number
	stateenum state; //state
	typeenum type; //type
public:
	buffer()
	{
		buf = new char[BUFSIZE];
		state = home;
		type = no_type;
		shift = 0;
		line = 1;
	}
	
	buffer(const buffer& a)
	{
		buf = new char[BUFSIZE];
		for (int i = 0; i < BUFSIZE; i++)
			buf[i] = a.buf[i];
		state = a.state;
		type = a.type;
		shift = a.shift;		
		line = a.line;
	}
	
	~buffer() {delete []buf;}
	
	int next(char c)
	{
		state = home;		
		do {			
			if ((c == EOF) && (state == home))
				return 0;
			feedme(c);
		} while (state != home);
		print();
		return 1;
	}
	
	//************useful functions**************
	//clears buf
	void clear()
	{
		shift = 0;
		buf[0] = '\0';
		type = no_type;
	}
	
	//add symbol to buf
	void add(char c)
	{
		if ((shift + 2) >= BUFSIZE)
			throw("The string is too long");
		else
		{			
			buf[shift] = c;
			buf[shift + 1] = '\0';
			shift++;
		}
	}
	
	//counts line
	void linecounter(char c)
	{
		if (c == '\n')
			line++;
	}
	
	//check for dividing symbols
	int symbolscheck(char c)
	{
		for (unsigned int i = 0; i < strlen(symbols); i++)
			if (c == symbols[i])
				return 1;
		return 0;
	}
	
	//prints all info
	void print()
	{
		if (type != no_type)
		{
			printf("Type: %10s", type_names[type]);		
			printf(", Line: %4d",line);
			printf(", Buffer: \"%10s\"\n", buf);
		}
	}
	//define type of keyword
	int keyword_type_set()
	{
		if (strcmp(buf, "if") == 0)
		{
			type = if_type;
			return 1;
		} else				
		if (strcmp(buf, "then") == 0)
		{
			type = then_type;
			return 1;
		} else
		if (strcmp(buf, "else") == 0)
		{
			type = else_type;
			return 1;
		} else
		if (strcmp(buf, "while") == 0)
		{
			type = while_type;
			return 1;
		} else
		if (strcmp(buf, "goto") == 0)
		{
			type = goto_type;
			return 1;
		} else
		if (strcmp(buf, "print") == 0)
		{
			type = goto_type;
			return 1;
		} else
		throw("Can't define state\n");
		return 0;
	}
	
	int getline() {return line;}
	
	void printbuf()	{printf("%s",buf);}	
	//************end of useful functions************
	
	//distribute "c" to its state
	void feedme(char c)
	{
		//counting line
		linecounter(c);
		//checking EOF
		if ((c == EOF) && (state != home))
			throw("EOF is too early");
		//choosing state
		switch (state)
		{
			case home:
				home_state(c);
				break;
			case number:
				number_state(c);
				break;
			case identifier:
				identifier_state(c);
				break;
			case keyword:
				keyword_state(c);
				break;
			case str:
				string_state(c);
				break;
			case assignment:
				assignment_state(c);
				break;
		}
	}
	
	//*************states***************
	int home_state(char c)
	{
		clear();
		add(c);
		type = no_type;
		if (symbolscheck(c))
			return 1;
			
		if ((c == '@') || (c == '$') || (c == '&'))
		{
			state = identifier;
			return 1;
		} else		
		if ((c >= '0') && (c <= '9'))
		{
			state = number;
			return 1;
		} else
		if (((c >= 'a') && (c <= 'z')) || 
		    ((c >= 'A') && (c <= 'Z')))
		{
		    state = keyword;
		    return 1;
		} else		   
		if (c == ':')
		{
			state = assignment;
			return 1;
		} else		
		if (c == '"')
		{
			//remove "
			clear();
			state = str;
			return 1;
		}
		return 0;
	}
	
	void number_state(char c)
	{
		if ((c >= '0') && (c <= '9'))
			add(c);
		else
			if (symbolscheck(c))
			{
				ungetc(c, stdin);
				type = number_type;
				state = home;
			}
			else
				throw("Wrong number format\n");
	}
	
	void identifier_state(char c)
	{
		if (((c >= 'a') && (c <= 'z')) || 
		    ((c >= 'A') && (c <= 'Z')) || 
		    ((c >= '0') && (c <= '9')))
		    add(c);
		else
			if (symbolscheck(c))
			{
				if (buf[0] == '$')
					type = variable_type;
				if (buf[0] == '@')
					type = label_type; 
				
				ungetc(c, stdin);
				state = home;
			}
			else
				throw("Wrong label format\n");		
	}
	
	void keyword_state(char c)
	{
		if (((c >= 'a') && (c <= 'z')) || 
		    ((c >= 'A') && (c <= 'Z')))
		    add(c);
		else
			if (symbolscheck(c))
			{				
				ungetc(c, stdin);
				keyword_type_set();
				state = home;				
			}
			else
				throw("Wrong keyword format\n");
	}
	
	void string_state(char c)
	{
		if (c == '"')
		{
			type = string_type;
			state = home;			
		}
		else 
			add(c);
	}
	
	void assignment_state(char c)
	{
		add(c);
		if (c != '=')
			throw("Wrong assignment format\n");
		else
		{
			type = assignment_type;
			state = home;			
		}
	}
	//*************end of states***************
};

int main()
{
	char c;
#if 1
	int fd;
	fd = open("test6.txt", O_RDONLY);
	dup2(fd, 0);
	if (fd == -1)
		throw("Can't open file");
#endif
	buffer buf;
	
	try
	{
		while (c != EOF)
		{
			c = getchar();
			buf.feedme(c);
			buf.print();
		}
	}
	catch (const char* s)
	{
		char c;
		printf("%s", s);
		printf("Line %d\n", buf.getline());
		buf.printbuf();
		while ((c = getchar()) != '\n')
			printf("%c",c);
	}
	return 0;
}
