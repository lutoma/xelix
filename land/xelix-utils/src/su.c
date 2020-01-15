/* Copyright © 2019 Lukas Martini
 *
 * This file is part of Xelix.
 *
 * Xelix is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Xelix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Xelix. If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <pwd.h>
#include <unistd.h>
#include "argparse.h"
#include "util.h"

static const char *const usage[] = {
    "su [options]",
    NULL,
};

int main(int argc, const char** argv) {
	char* user = "root";
	struct argparse_option options[] = {
		OPT_HELP(),
		OPT_STRING('u', "user", &user, "user to authenticate as"),
        OPT_END(),
	};

    struct argparse argparse;
    argparse_init(&argparse, options, usage, 0);

    argparse_describe(&argparse, "Change the effective user ID and group ID to that of <user>.",
    	"\nsu is part of xelix-utils. Please report bugs to <hello@lutoma.org>.");
    argc = argparse_parse(&argparse, argc, argv);

    int uid = getuid();
    struct passwd* pwd = NULL;
    if(uid == 0) {
    	pwd = getpwnam(user);
    } else {
		pwd = do_auth(user);
	}

	if(pwd) {
		run_shell(pwd, false);
	} else {
		fprintf(stderr, "su: Authentication failure.\n");
	}
}
