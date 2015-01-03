//
//  main.swift
//  Perian
//
//  Created by C.W. Betts on 1/3/15.
//  Copyright (c) 2015 Perian. All rights reserved.
//

import Cocoa

extension timespec {
	private init() {
		tv_sec = 0
		tv_nsec = 0
	}
}

let lockPath = "/tmp/PerianUpdateLock"

private var fp: Int32 = -1;


fp = open(lockPath, O_CREAT | O_EXCL, S_IWUSR | S_IRUSR);
if(fp == -1)
{
/*     var st_dev: dev_t
var st_mode: mode_t
var st_nlink: nlink_t
var st_ino: __darwin_ino64_t
var st_uid: uid_t
var st_gid: gid_t
var st_rdev: dev_t
var st_atimespec: timespec
var st_mtimespec: timespec
var st_ctimespec: timespec
var st_birthtimespec: timespec
var st_size: off_t
var st_blocks: blkcnt_t
var st_blksize: blksize_t
var st_flags: __uint32_t
var st_gen: __uint32_t
var st_lspare: __int32_t
var st_qspare: (__int64_t, __int64_t)
*/

	var lockfile: stat = stat(st_dev: 0, st_mode: 0, st_nlink: 0, st_ino: 0, st_uid: 0, st_gid: 0, st_rdev: 0, st_atimespec: timespec(), st_mtimespec: timespec(), st_ctimespec: timespec(), st_birthtimespec: timespec(), st_size: 0, st_blocks: 0, st_blksize: 0, st_flags: 0, st_gen: 0, st_lspare: 0, st_qspare: (0, 0))
	var current = time(nil);
	if (stat(lockPath, &lockfile) != 0) {
		if(lockfile.st_ctimespec.tv_sec + 60 * 60 > current)  //Only pay attention to lock file if it is no more than an hour old
		{
			unlink(lockPath);
			fp = open(lockPath, O_CREAT | O_EXCL | O_EXLOCK, S_IWUSR | S_IRUSR);
		}
	}
}
if(fp == -1) {
exit(0)
}

atexit_b { () -> Void in
	close(fp);
	unlink(lockPath);
}

//NSApplicationm
