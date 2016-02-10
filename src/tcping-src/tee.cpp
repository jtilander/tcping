
#include <fstream>
#include <stdio.h>
#include <stdarg.h>

/*

Don't make fun of me, Unix guys.  I know about `which tee`.  Tee isn't always available on windows systems, however.

*/

class tee
{
public:
	tee();
	~tee(void);
	void Open(char* filename);
	void Close();
	void p(const char* text);
	void pf(const char* format, ...);
	void enable(bool onoff);

private:
	std::ofstream outfile;
	int flag;
	bool enable_output;
};


tee::tee()
{
	flag = 0;
	enable_output = true;
}

tee::~tee()
{
	this->Close();
}

void tee::Open(char*filename)
{
	if (flag != 0) {
		outfile.close();
	}
	outfile.open(filename);
	flag = 1;
}

void tee::Close()
{
	if (flag != 0) {
		outfile.close();
	}
	flag = 0;
}

void tee::p(const char* text)
{
	if (enable_output == false) {
		return;
	}

	printf(text);
	if (flag == 1) {
		outfile << text;
		outfile.flush();
	}
	fflush(stdout);
}

void tee::pf(const char* format, ...)
{
	if (enable_output == false) {
		return;
	}

	char buffer[256];
	va_list args;
	va_start(args, format);
	//vsprintf(buffer, format, args);
	vsprintf_s(buffer, 256, format, args);


	va_end(args);

	this->p(buffer);
}

void tee::enable(bool onoff){
	enable_output = onoff;
}







