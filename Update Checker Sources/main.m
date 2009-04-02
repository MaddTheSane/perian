/*
 * Perian Update Checker main.m
 * Created by Augie Fackler on 1/6/07.
 *
 * This file is part of Perian.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#import <Cocoa/Cocoa.h>
#include <sys/stat.h>
#include <unistd.h>

#define lockPath "/tmp/PerianUpdateLock"

static int fp = -1;

static void deleteLock()
{
	close(fp);
	unlink(lockPath);
}

int main(int argc, char *argv[])
{
	fp = open(lockPath, O_CREAT | O_EXCL);
	if(fp == -1)
	{
		struct stat lockfile;
		time_t current = time(NULL);
		if(stat(lockPath, &lockfile))
		{
			if(lockfile.st_ctimespec.tv_sec + 60*60 > current)  //Only pay attention to lock file if it is no more than an hour old
			{
				unlink(lockPath);
				fp = open(lockPath, O_CREAT | O_EXCL | O_EXLOCK);
			}
		}		
	}
	if(fp == -1)
		return 0;
	
	atexit(deleteLock);

	int ret = NSApplicationMain(argc,  (const char **) argv);
	
	return ret;
}
