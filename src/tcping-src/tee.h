#include <fstream>
#include <stdio.h>

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

