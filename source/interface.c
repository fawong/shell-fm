/*
	Copyright (C) 2006 by Jonas Kramer
	Published under the terms of the GNU General Public License (GPL).
*/

#define _GNU_SOURCE

#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/time.h>
#include <signal.h>
#include <stdarg.h>
#include <ctype.h>

#include "service.h"
#include "hash.h"
#include "interface.h"
#include "autoban.h"
#include "settings.h"
#include "http.h"
#include "split.h"
#include "bookmark.h"
#include "radio.h"
#include "md5.h"
#include "submit.h"
#include "readline.h"
#include "xmlrpc.h"
#include "recommend.h"
#include "util.h"
#include "mix.h"

#include "globals.h"


struct hash track;

void interface(int interactive) {
	if(interactive) {
		int key, result;
		char customkey[8] = { 0 }, * marked = NULL;
		
		canon(0);
		fflush(stderr);
		if((key = fetchkey(1000000)) == -1)
			return;

		if(key == 27) {
			int ch;
			while((ch = fetchkey(100000)) != -1 && !strchr("ABCDEFGHMPQRSZojmk~", ch));
			return;
		}

		switch(key) {
			case 'l':
				puts(rate("L") ? "Loved." : "Sorry, failed.");
				break;

			case 'U':
				puts(rate("U") ? "Unloved." : "Sorry, failed.");
				break;

			case 'B':
				puts(rate("B") ? "Banned." : "Sorry, failed.");
				kill(playfork, SIGUSR1);
				break;

			case 'n':
				rate("S");
				break;

			case 'Q':
				unlink(rcpath("session"));
				exit(EXIT_SUCCESS);

			case 'i':
				if(playfork) {
					const char * path = rcpath("i-template");
					if(path && !access(path, R_OK)) {
						char ** template = slurp(path);
						if(template != NULL) {
							unsigned n = 0;
							while(template[n]) {
								puts(meta(template[n], !0));
								free(template[n++]);
							}
							free(template);
						}
					}
					else {
						puts(meta("Track:    \"%t\" (%T)", !0));
						puts(meta("Artist:   \"%a\" (%A)", !0));
						puts(meta("Album:    \"%l\" (%L)", !0));
						puts(meta("Station:  %s", !0));
					}
				}
				break;

			case 'r':
				radioprompt("radio url> ");
				break;

			case 'd':
				toggle(DISCOVERY);
				printf("Discovery mode %s.\n", enabled(DISCOVERY) ? "enabled" : "disabled");
				if(playfork) {
					printf(
						"%u track(s) left to play/skip until change comes into affect.\n",
						playlist.left
					);
				}
				break;

			case 'A':
				printf(meta("Really ban all tracks by artist %a? [yN]", !0));
				fflush(stdout);
				if(fetchkey(5000000) != 'y')
					puts("\nAbort.");
				else if(autoban(value(& track, "creator"))) {
					printf("\n%s banned.\n", meta("%a", !0));
					rate("B");
				}
				fflush(stdout);
				break;

			case 'a':
				result = xmlrpc(
					"addTrackToUserPlaylist", "ss",
					value(& track, "creator"),
					value(& track, "title")
				);
				
				puts(result ? "Added to playlist." : "Sorry, failed.");
				break;

			case 'P':
				toggle(RTP);
				printf("%s RTP.\n", enabled(RTP) ? "Enabled" : "Disabled");
				break;

			case 'f':
				if(playfork) {
					const char * uri = meta("lastfm://artist/%a/fans", 0);
					if(haskey(& rc, "delay-change")) {
						puts("\rDelayed.");
						nextstation = strdup(uri);
					}
					else {
						station(uri);
					}
				}
				break;
				
			case 's':
				if(playfork) {
					const char * uri = meta("lastfm://artist/%a/similarartists", 0);
					if(haskey(& rc, "delay-change")) {
						puts("\rDelayed.");
						nextstation = strdup(uri);
					}
					else {
						station(uri);
					}
				}
				break;

			case 'h':
				printmarks();
				break;

			case 'H':
				if(playfork && currentStation) {
					puts("What number do you want to bookmark this stream as? [0-9]");
					key = fetchkey(5000000);
					setmark(currentStation, key - 0x30);
				}
				break;

			case 'S':
				if(playfork) {
					enable(STOPPED);
					kill(playfork, SIGUSR1);
				}
				break;

			case 'T':
				if(playfork)
					tag(track);
				break;

			case 'p':
				if(playfork)
					kill(playfork, pausetime ? SIGCONT : SIGSTOP);
				break;

			case 'R':
				if(playfork) {
					recommend(track);
				}
				break;

			case '+':
				adjust(+STEP, VOL);
				break;

			case '-':
				adjust(-STEP, VOL);
				break;

			case '*':
				adjust(+STEP, PCM);
				break;

			case '/':
				adjust(-STEP, PCM);
				break;

      case '?':
				puts("a = add the track to the playlist");
        puts("A = autoban artist");
        puts("B = ban Track");
        puts("d = discovery mode");
        puts("f = fan Station");
				puts("h = list bookmarks");
				puts("H = bookmark current radio");
        puts("i = current track information");
        puts("l = love track");
        puts("n = skip track");
				puts("p = pause");
        puts("P = enable/disable RTP");
        puts("Q = quit");
        puts("r = change radio station");
				puts("R = recommend track/artist/album");
        puts("S = stop");
        puts("s = similiar artist");
        puts("T = tag track/artist/album");
        puts("U = unlock track");
				puts("+ = increase volume (vol)");
				puts("- = decrease volume (vol)");
				puts("* = increase volume (pcm)");
				puts("/ = decrease volume (pcm)");
        break;

			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				if((marked = getmark(key - 0x30))) {
					station(marked);
					free(marked);
				} else {
					puts("Bookmark not defined.");
				}
				break;

			default:
				snprintf(customkey, sizeof(customkey), "key0x%02X", key & 0xFF);
				if(haskey(& rc, customkey))
					run(meta(value(& rc, customkey), 0));
		}
	}
}

int fetchkey(unsigned nsec) {
	fd_set fdset;
	struct timeval tv;

	FD_ZERO(& fdset);
	FD_SET(fileno(stdin), & fdset);

	tv.tv_usec = nsec % 1000000;
	tv.tv_sec = nsec / 1000000;

	if(select(fileno(stdin) + 1, & fdset, NULL, NULL, & tv) > 0) {
		char ch = -1;
		if(read(fileno(stdin), & ch, sizeof(char)) == sizeof(char))
			return ch;
	}

	return -1;
}


#define remn (sizeof(string) - length - 1)
const char * meta(const char * fmt, int colored) {
	static char string[4096];
	unsigned length = 0, x = 0;

        /* Switch off coloring when in batch mode */
        colored = (colored && !(batch));

	if(!fmt)
		return NULL;
	
	memset(string, 0, sizeof(string));
	
	while(fmt[x] && remn > 0) {
		if(fmt[x] != 0x25)
			string[length++] = fmt[x++];
		else if(fmt[++x]) {
			const char * keys [] = {
				"acreator",
				"ttitle",
				"lalbum",
				"Aartistpage",
				"Ttrackpage",
				"Lalbumpage",
				"dduration",
				"sstation",
				"SstationURL",
				"Rremain"
			};

			register unsigned i = sizeof(keys) / sizeof(char *);
			
			while(i--)
				if(fmt[x] == keys[i][0]) {
					const char * val = value(& track, keys[i] + 1), * color = NULL;
					if(colored) {
						char colorkey[64] = { 0 };
						snprintf(colorkey, sizeof(colorkey), "%c-color", keys[i][0]);
						color = value(& rc, colorkey);
						if(color) {
							// Strip leading spaces from end of color (Author: Ondrej Novy)
							char * color_st = strdup(color);
							size_t len = strlen(color_st) - 1;
							while(isspace(color_st[len]) && len > 0) {
								color_st[len] = 0;
								len--;
							}
							length += snprintf(string + length, remn, "\x1B[%sm", color_st);
							free(color_st);
						}
					}
					length = strlen(strncat(string, val ? val : "(unknown)", remn));
					if(color) {
						length = strlen(strncat(string, "\x1B[0m", remn));
					}
					break;
				}
			++x;
		}
	}

	return string;
}
#undef remn

void run(const char * cmd) {
	if(!fork()) {
		FILE * fd = popen(cmd, "r");
		if(!fd)
			exit(EXIT_FAILURE);
		else {
			int ch;
			
			while((ch = fgetc(fd)) != EOF)
				fputc(ch, stdout);

			fflush(stdout);
			_exit(pclose(fd));
		}
	}
}


int rate(const char * rating) {
	if(playfork && rating != NULL) {
		set(& track, "rating", rating);

		switch(rating[0]) {
			case 'B':
				kill(playfork, SIGUSR1);
				return xmlrpc(
					"banTrack",
					"ss",
					value(& track, "creator"),
					value(& track, "title")
				);

			case 'L':
				return xmlrpc(
					"loveTrack",
					"ss",
					value(& track, "creator"),
					value(& track, "title")
				);

			case 'U':
				return xmlrpc(
					"unLoveTrack",
					"ss",
					value(& track, "creator"),
					value(& track, "title")
				);

			case 'S':
				kill(playfork, SIGUSR1);
				return !0;
		}
	}

	return 0;
}
