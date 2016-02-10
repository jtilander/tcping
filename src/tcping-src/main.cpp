/***********************************************************************
tcping.exe -- A tcp probe utility
Copyright (C) 2005-2013 Eli Fulkerson

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

----------------------------------------------------------------------

Other license terms may be negotiable.  Contact the author if you would
like a copy that is licensed differently.

Contact information (as well as this program) lives at http://www.elifulkerson.com

----------------------------------------------------------------------

This application includes public domain code from the Winsock Programmer's FAQ:
  http://www.tangentsoft.net/wskfaq/
... and a big thank you to the maintainers and contributers therein.

***********************************************************************/

const char *TCPING_VERSION = "0.31";
const char *TCPING_DATE = "Dec 14 2015";

#pragma comment(lib, "Ws2_32.lib")

#include <winsock2.h>
#include <stdlib.h>
#include <iostream>
#include <time.h>

#include "tee.h"
#include "tcping.h"


using namespace std;

void usage(int argc, char* argv[]) {
    cout << "--------------------------------------------------------------" << endl;
    cout << "tcping.exe by Eli Fulkerson " << endl;
    cout << "Please see http://www.elifulkerson.com/projects/ for updates. " << endl;
    cout << "--------------------------------------------------------------" << endl;
    cout << endl;
    cout << "Usage: " << argv[0] << " [-flags] server-address [server-port]" << endl << endl;
    cout << "Usage (full): " << argv[0] << " [-t] [-d] [-i interval] [-n times] [-w ms] [-b n] [-r times] [-s] [-v] [-j] [--file] [--tee filename] [-h] [-u] [--post] [--head] [-f] server-address " << "[server-port]" << endl << endl;
    cout << " -t     : ping continuously until stopped via control-c" << endl;
    cout << " -n 5   : for instance, send 5 pings" << endl;
    cout << " -i 5   : for instance, ping every 5 seconds" << endl;
    cout << " -w 0.5 : for instance, wait 0.5 seconds for a response" << endl;
    cout << " -d     : include date and time on each line" << endl;
    cout << " -b 1   : enable beeps (1 for on-down, 2 for on-up," << endl;
    cout << "                        3 for on-change, 4 for always)" << endl;
    cout << " -r 5   : for instance, relookup the hostname every 5 pings" << endl;
    cout << " -s     : automatically exit on a successful ping"<<endl;                  //[Modification 14 Apr 2011 by Michael Bray, mbray@presidio.com]
    cout << " -v     : print version and exit" << endl;
    cout << " -j     : include jitter, using default rolling average"<< endl;
	cout << " -js 5  : include jitter, with a rolling average size of (for instance) 5." << endl;
	cout << " --tee  : mirror output to a filename specified after '--tee'" << endl;
	cout << " -4     : prefer ipv4" << endl;
	cout << " -6     : prefer ipv6" << endl;
	cout << " -c     : only show an output line on changed state" << endl;
	cout << " --file : treat the \"server-address\" as a filename instead, loop through file line by line" << endl;
	cout << "          Note: --file is incompatible with options such as -j and -c as it is looping through different targets" << endl;
	cout << " -g 5   : for instance, give up if we fail 5 times in a row" << endl;
    cout << endl << "HTTP Options:" << endl;
    cout << " -h     : HTTP mode (use url without http:// for server-address)" << endl;
    cout << " -u     : include target URL on each line" << endl;
    cout << " --post : use POST rather than GET (may avoid caching)" << endl;
    cout << " --head : use HEAD rather than GET" << endl;
	cout << " --proxy-server : specify a proxy server " << endl;
	cout << " --proxy-port   : specify a proxy port " << endl;
	cout << " --proxy-credentials : specify 'Proxy-Authorization: Basic' header in format username:password" << endl;
    cout << endl << "Debug Options:" << endl;
    cout << " -f     : force tcping to send at least one byte" << endl;
	cout << " --header : include a header with original args and date.  Implied if using --tee." << endl;
    cout << endl << "\tIf you don't pass server-port, it defaults to " << kDefaultServerPort << "." << endl;

}


int main(int argc, char* argv[]) {

    // Do we have enough command line arguments?
    if (argc < 2) {
        usage(argc, argv);
        return 1;
    }

    int times_to_ping = 4;
    int offset = 0;  // because I don't feel like writing a whole command line parsing thing, I just want to accept an optional -t.  // well, that got out of hand quickly didn't it? -Future Eli
    double ping_interval = 1;
    int include_timestamp = 0;
    int beep_mode = 0;  // 0 is off, 1 is down, 2 is up, 3 is on change, 4 is constantly
    int ping_timeout = 2000;
    int relookup_interval = -1;
    int auto_exit_on_success = 0;
    int force_send_byte = 0;

    int include_url = 0;
    int use_http = 0;
    int http_cmd = 0;

    int include_jitter = 0;
    int jitter_sample_size = 0;

    int only_changes = 0;

    // for http mode
    char *serverptr;
    char *docptr = NULL;
    char server[2048];
    char document[2048];

    // for --tee
    char logfile[256];
    int use_logfile = 0;
	int show_arg_header = 0;

    // preferred IP version
    int ipv = 0;

	// http proxy server and port
	int proxy_port = 3128;
	char proxy_server[2048];
	proxy_server[0] = 0;

	char proxy_credentials[2048];
	int using_credentials = 0;

	// Flags for "read from filename" support
	int no_statistics = 0;  // no_statistics flag kills the statistics finale in the cases where we are reading entries from a file
	int reading_from_file = 0;  // setting this flag so we can mangle the other settings against it post parse.  For instance, it moves the meaning of -n and -t
	char urlfile[256];
	int file_times_to_loop = 1;
	bool file_loop_count_was_specific = false;   // ugh, since we are taking over the -n and -t options, but we don't want a default of 4 but we *do* want 4 if they specified 4

	int giveup_count = 0;

	for (int x = 0; x < argc; x++) {

		if (!strcmp(argv[x], "/?") || !strcmp(argv[x], "?") || !strcmp(argv[x], "--help") || !strcmp(argv[x], "-help")) {
			usage(argc, argv);
			return 1;
		}

		if (!strcmp(argv[x], "--proxy-port")) {
			proxy_port = atoi(argv[x + 1]);
			offset = x + 1;
		}

		if (!strcmp(argv[x], "--proxy-server")) {
			sprintf_s(proxy_server, sizeof(proxy_server), argv[x + 1]);
			offset = x + 1;
		}

		if (!strcmp(argv[x], "--proxy-credentials")) {
			sprintf_s(proxy_credentials, sizeof(proxy_credentials), argv[x + 1]);
			using_credentials = 1;
			offset = x + 1;
		}

		// force IPv4
		if (!strcmp(argv[x], "-4")) {
			ipv = 4;
			offset = x;
		}

		// force IPv6
		if (!strcmp(argv[x], "-6")) {
			ipv = 6;
			offset = x;
		}

		// ping continuously
		if (!strcmp(argv[x], "-t")) {
			times_to_ping = -1;
			file_loop_count_was_specific = true;
			offset = x;
			cout << endl << "** Pinging continuously.  Press control-c to stop **" << endl;
		}

		// Number of times to ping
		if (!strcmp(argv[x], "-n")) {
			times_to_ping = atoi(argv[x + 1]);
			file_loop_count_was_specific = true;
			offset = x + 1;
		}

		// Give up
		if (!strcmp(argv[x], "-g")) {
			giveup_count = atoi(argv[x + 1]);
			offset = x + 1;
		}

		// exit on first successful ping
		if (!strcmp(argv[x], "-s")) {
			auto_exit_on_success = 1;
			offset = x;
		}

		if (!strcmp(argv[x], "--header")) {
			show_arg_header = 1;
			offset = x;
		}

		// tee to a log file
		if (!strcmp(argv[x], "--tee")) {
			strcpy_s(logfile, sizeof(logfile), static_cast<const char*>(argv[x + 1]));
			offset = x + 1;
			use_logfile = 1;
			show_arg_header = 1;
		}

		// read from a text file
		if (!strcmp(argv[x], "--file")) {
			offset = x;
			no_statistics = 1;
			reading_from_file = 1;
		}

        // http mode
        if (!strcmp(argv[x], "-h")) {
            use_http = 1;
            offset = x;
        }

        // http mode - use get
        if (!strcmp(argv[x], "--get")) {
            use_http = 1; //implied
            http_cmd = HTTP_GET;
            offset = x;
        }

        // http mode - use head
        if (!strcmp(argv[x], "--head")) {
            use_http = 1; //implied
            http_cmd = HTTP_HEAD;
            offset = x;
        }

        // http mode - use post
        if (!strcmp(argv[x], "--post")) {
            use_http = 1; //implied
            http_cmd = HTTP_POST;
            offset = x;
        }

        // include url per line
        if (!strcmp(argv[x], "-u")) {
            include_url = 1;
            offset = x;
        }

        // force send a byte
        if (!strcmp(argv[x], "-f")) {
            force_send_byte = 1;
            offset = x;
        }

        // interval between pings
        if (!strcmp(argv[x], "-i")) {
            ping_interval = atof(argv[x+1]);
            offset = x+1;
        }

        // wait for response
        if (!strcmp(argv[x], "-w")) {
			ping_timeout = (int)(1000 * atof(argv[x + 1]));
            offset = x+1;
        }

        // optional datetimestamp output
        if (!strcmp(argv[x], "-d")) {
            include_timestamp = 1;
            offset = x;
        }

        // optional jitter output
        if (!strcmp(argv[x], "-j")) {
            include_jitter = 1;
            offset = x;
		}
     
		// optional jitter output (sample size)
		if (!strcmp(argv[x], "-js")) {
            include_jitter = 1;
            offset = x;

            // obnoxious special casing if they actually specify the default 0
            if (!strcmp(argv[x+1], "0")) {
                jitter_sample_size = 0;
                offset = x+1;
            } else {
                if (atoi(argv[x+1]) == 0) {
                    offset = x;
                } else {
                    jitter_sample_size = atoi(argv[x+1]);
                    offset = x+1;
                }
            }
            //			cout << "offset coming out "<< offset << endl;
        }

        // optional hostname re-lookup
        if (!strcmp(argv[x], "-r")) {
            relookup_interval = atoi(argv[x+1]);
            offset = x+1;
        }
		
		 // optional output minimization
        if (!strcmp(argv[x], "-c")) {
            only_changes = 1;
            offset = x;
			cout << endl << "** Only displaying output for state changes. **" << endl;
        }

        // optional beepage
        if (!strcmp (argv[x], "-b")) {
            beep_mode = atoi(argv[x+1]);
            offset = x+1;
            switch (beep_mode) {
            case 0:
                break;
            case 1:
                cout << endl << "** Beeping on \"down\" - (two beeps) **" << endl;
                break;
            case 2:
                cout << endl << "** Beeping on \"up\"  - (one beep) **" << endl;
                break;
            case 3:
                cout << endl << "** Beeping on \"change\" - (one beep up, two beeps down) **" << endl;
                break;
            case 4:
                cout << endl << "** Beeping constantly - (one beep up, two beeps down) **" << endl;
                break;
            }

        }

        // dump version and quit
        if (!strcmp(argv[x], "-v") || !strcmp(argv[x], "--version") ) {
            //cout << "tcping.exe 0.30 Nov 13 2015" << endl;
			cout << "tcping.exe " << TCPING_VERSION << " " << TCPING_DATE << endl;
            cout << "compiled: " << __DATE__ << " " << __TIME__ <<  endl;
            cout << endl;
            cout << "tcping.exe by Eli Fulkerson " << endl;
            cout << "Please see http://www.elifulkerson.com/projects/ for updates. " << endl;
            cout << endl;
            cout << "-s option contributed 14 Apr 2011 by Michael Bray, mbray@presidio.com" << endl;
			cout << "includes base64.cpp Copyright (C) 2004-2008 René Nyffenegger" << endl;
            return 1;
        }
	}

	// open our logfile, if applicable
	tee out;
	if (use_logfile == 1 && logfile != NULL) {
		out.Open(logfile);
	}



	if (show_arg_header == 1) {
		out.p("-----------------------------------------------------------------\n");
		// print out the args
		out.p("args: ");
		for (int x = 0; x < argc; x++) {
			out.pf("%s ", argv[x]);
		}
		out.p("\n");


		// and the date

		time_t rawtime;
		struct tm  timeinfo;
		char dateStr[11];
		char timeStr[9];

		errno_t err;

		_strtime_s(timeStr, sizeof(timeStr));

		time(&rawtime);

		err = localtime_s(&timeinfo, &rawtime);
		strftime(dateStr, 11, "%Y:%m:%d", &timeinfo);
		out.pf("date: %s %s\n", dateStr, timeStr);

		// and the attrib
		out.pf("tcping.exe v%s: http://www.elifulkerson.com/projects/tcping.php\n", TCPING_VERSION);
		out.p("-----------------------------------------------------------------\n");

	}





	// Get host and (optionally) port from the command line

	char* pcHost = "";
	//char pcHost[2048] = "";
	
    if (argc >= 2 + offset) {
		if (!reading_from_file) {
			pcHost = argv[1 + offset];
		}
		else {
			strcpy_s(urlfile, sizeof(urlfile), static_cast<const char*>(argv[offset + 1]));
		}


    } else {
			cout << "Check the last flag before server-address.  Did you specify a flag and forget its argument?" << endl;
			return 1;
    }

    int nPort = kDefaultServerPort;
    if (argc >= 3 + offset) {
        nPort = atoi(argv[2 + offset]);
    }

    // Do a little sanity checking because we're anal.
    int nNumArgsIgnored = (argc - 3 - offset);
    if (nNumArgsIgnored > 0) {
        cout << nNumArgsIgnored << " extra argument" << (nNumArgsIgnored == 1 ? "" : "s") << " ignored.  FYI." << endl;
    }

    if (use_http == 1 && reading_from_file == 0) {   //added reading from file because if we are doing multiple http this message is just spam.
        serverptr = strchr(pcHost, ':');
        if (serverptr != NULL) {
            ++serverptr;
            ++serverptr;
            ++serverptr;
        } else {
            serverptr = pcHost;
        }

        docptr = strchr(serverptr, '/');
        if (docptr != NULL) {
            *docptr = '\0';
            ++docptr;

			strcpy_s(server, sizeof(server), static_cast<const char*>(serverptr));
			strcpy_s(document, sizeof(document), static_cast<const char*>(docptr));
        } else {
			strcpy_s(server, sizeof(server), static_cast<const char*>(serverptr));
            document[0] = '\0';
        }

		out.pf("\n** Requesting %s from %s:\n", document, server);
		out.p("(for various reasons, kbit/s is an approximation)\n");
    }

    SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);

    // Start Winsock up
    WSAData wsaData;
    int nCode;
    if ((nCode = WSAStartup(MAKEWORD(1, 1), &wsaData)) != 0) {
        cout << "WSAStartup() returned error code " << nCode << "." << endl;
        return 255;
    }

    // Call the main example routine.
	int retval;

	out.p("\n");

	if (!reading_from_file) {
		retval = DoWinsock_Single(pcHost, nPort, times_to_ping, ping_interval, include_timestamp, beep_mode, ping_timeout, relookup_interval, auto_exit_on_success, force_send_byte, include_url, use_http, docptr, http_cmd, include_jitter, jitter_sample_size, logfile, use_logfile, ipv, proxy_server, proxy_port, using_credentials, proxy_credentials, only_changes, no_statistics, giveup_count, out);
	}
	else {
		if (file_loop_count_was_specific) {
			file_times_to_loop = times_to_ping;
		}
		times_to_ping = 1;
		retval = DoWinsock_Multi(pcHost, nPort, times_to_ping, ping_interval, include_timestamp, beep_mode, ping_timeout, relookup_interval, auto_exit_on_success, force_send_byte, include_url, use_http, docptr, http_cmd, include_jitter, jitter_sample_size, logfile, use_logfile, ipv, proxy_server, proxy_port, using_credentials, proxy_credentials, only_changes, no_statistics, giveup_count, file_times_to_loop, urlfile, out);
	}

    // Shut Winsock back down and take off.
    WSACleanup();
    return retval;
}

