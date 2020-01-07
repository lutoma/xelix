/* Adaptation of libc/unix/getpwent.c 2018-10-10 lutoma */

#include <stdio.h>
#include <sys/types.h>
#include <pwd.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <grp.h>

static struct group gr_group;	/* group structure */
static FILE *group_fp;

static char logname[8];
static char password[1024];
static char users[1024];

struct group *
getgrnam (name)
     const char *name;
{
  FILE *fp;
  char buf[1024];

  if ((fp = fopen ("/etc/group", "r")) == NULL)
    {
      return NULL;
    }

  while (fgets (buf, sizeof (buf), fp))
    {
      sscanf (buf, "%[^:]:%[^:]:%d:%s\n",
	      logname, password, &gr_group.gr_gid, users);

      gr_group.gr_name = logname;
      gr_group.gr_mem = (char**)&users;

      if (!strcmp (logname, name))
	{
	  fclose (fp);
	  return &gr_group;
	}
    }
  fclose (fp);
  return NULL;
}

struct group *
getgrgid (uid_t uid)
{
  FILE *fp;
  char buf[1024];

  if ((fp = fopen ("/etc/group", "r")) == NULL)
    {
      return NULL;
    }

  while (fgets (buf, sizeof (buf), fp))
    {
      sscanf (buf, "%[^:]:%[^:]:%d:%s\n",
        logname, password, &gr_group.gr_gid, users);

      gr_group.gr_name = logname;
      gr_group.gr_mem = (char**)&users;

      if (uid == gr_group.gr_gid)
	{
	  fclose (fp);
	  return &gr_group;
	}
    }
  fclose (fp);
  return NULL;
}

struct group *
getgrent ()
{
  char buf[1024];

  if (group_fp == NULL)
    return NULL;

  if (fgets (buf, sizeof (buf), group_fp) == NULL)
    return NULL;

  sscanf (buf, "%[^:]:%[^:]:%d:%s\n",
    logname, password, &gr_group.gr_gid, users);

  gr_group.gr_name = logname;
  gr_group.gr_mem = (char**)&users;

  return &gr_group;
}

void
setgrent ()
{
  if (group_fp != NULL)
    fclose (group_fp);

  group_fp = fopen ("/etc/group", "r");
}

void
endgrent ()
{
  if (group_fp != NULL)
    fclose (group_fp);
}
