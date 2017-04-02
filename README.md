Deadlock detector GDB plugin
============================

Installation
------------
- Append the following to your ~/.gdbinit
    ```
    python import sys, os; sys.path.append("/path/to/this_dir")
    python import gdb_info_mutex
    ```

- You will also need debug symbol for you version of libc, you can install them on debian or on ubuntu with the following command:
    `apt-get install libc6-dbg`

Usage
-----
In gdb type the following command: `info mutex`

Acknowledgements
----------------
I'd like to thank Tom Tromey for his work which is the base of this deadlock detector utility!
http://www.sourceware.org/ml/archer/2010-q3/msg00024.html

Tested platforms
----------------
- ubuntu 16.04  / x86_64 / gdb-7.11.1 / python-3.5.2 / gcc-5.4.0 - works
- ubuntu 14.04  / x86_64 / gdb-7.7.1  / python-3.4.0 / gcc-4.8.2 - works
- debian 7.2    /   i386 / gdb-7.4.1  / python-2.7.3 / gcc-4.7.2 - works
- debian 6.0.8  /   i386 / gdb-7.3.1  / python-2.6.6 / gcc-4.4.5 - works
- debian 6.0.10 /   i386 / gdb-7.0.1  / python-2.6.6 / gcc-4.4.5 - doesn't work (no gdb.inferiors)

Example run
-----------
```shell
$ gdb ./deadlock
Reading symbols from ./deadlock...done.

(gdb) run
Starting program: ./deadlock
[Thread debugging using libthread_db enabled]
Using host libthread_db library "/lib/x86_64-linux-gnu/libthread_db.so.1".
[New Thread 0x7ffff6f4f700 (LWP 13576)]
[New Thread 0x7ffff674e700 (LWP 13577)]
thread(140737336637184) started
thread(140737328244480) started
[New Thread 0x7ffff5f4d700 (LWP 13578)]
[New Thread 0x7ffff574c700 (LWP 13579)]
watchdog(140737311459072) started
thread(140737328244480) loop 1 begin
thread(140737328244480) loop 1 all locks acquired
thread(140737336637184) loop 1 begin
thread(140737319851776) started
thread(140737319851776) loop 1 begin
[New Thread 0x7ffff4f4b700 (LWP 13580)]
thread(140737303066368) started
thread(140737303066368) loop 1 begin
[New Thread 0x7fffdffff700 (LWP 13581)]
thread(140736951482112) started
thread(140736951482112) loop 1 begin
[New Thread 0x7fffdf7fe700 (LWP 13582)]
thread(140736943089408) started
thread(140736943089408) loop 1 begin
[New Thread 0x7fffdeffd700 (LWP 13583)]
thread(140736934696704) started
thread(140736934696704) loop 1 begin
[New Thread 0x7fffde7fc700 (LWP 13584)]
thread(140736926304000) started
thread(140736926304000) loop 1 begin
[New Thread 0x7fffddffb700 (LWP 13585)]
thread(140736917911296) started
thread(140736917911296) loop 1 begin
[New Thread 0x7fffdd7fa700 (LWP 13586)]
thread(140736909518592) started
thread(140736909518592) loop 1 begin
thread(140737328244480) loop 1 end
thread(140737328244480) loop 2 begin
thread(140737336637184) loop 1 all locks acquired
thread(140737336637184) loop 1 end
thread(140737336637184) loop 2 begin
watchdog(140737311459072) unresponsive thread: 140737319851776
watchdog(140737311459072) unresponsive thread: 140736909518592
watchdog(140737311459072) unresponsive thread: 140736917911296
watchdog(140737311459072) unresponsive thread: 140736926304000
watchdog(140737311459072) unresponsive thread: 140736934696704
watchdog(140737311459072) unresponsive thread: 140736943089408
watchdog(140737311459072) unresponsive thread: 140736951482112
watchdog(140737311459072) unresponsive thread: 140737303066368
watchdog(140737311459072) unresponsive thread: 140737319851776
watchdog(140737311459072) unresponsive thread: 140737328244480
watchdog(140737311459072) unresponsive thread: 140737336637184
watchdog(140737311459072) finished, allAlive: false, empty: false
terminate called after throwing an instance of 'Watchdog::Timeout'
  what():  Watchdog timeout

Thread 5 "deadlock" received signal SIGABRT, Aborted.
[Switching to Thread 0x7ffff574c700 (LWP 13579)]
0x00007ffff728e428 in __GI_raise (sig=sig@entry=6) at ../sysdeps/unix/sysv/linux/raise.c:54
54	../sysdeps/unix/sysv/linux/raise.c: No such file or directory.

(gdb) info mutex
Mutex 0x624d10:
  Owner   6 (LWP 13580)  owns: this 0x624c48
  Thread 12 (LWP 13586) 
  Thread  9 (LWP 13583)  owns: 0x624c70

Mutex 0x624db0:
  Owner   4 (LWP 13578)  owns: this 0x624ec8
  Thread 11 (LWP 13585) 

Mutex 0x624ec8:
  Owner   4 (LWP 13578)  owns: 0x624db0 this
  Thread  6 (LWP 13580)  owns: 0x624d10 0x624c48

Mutex 0x624cc0:
  Owner  10 (LWP 13584)  owns: 0x624ea0 this
  Thread  3 (LWP 13577) 

Mutex 0x624d88:
  Owner   7 (LWP 13581)  owns: this
  Thread 10 (LWP 13584)  owns: 0x624ea0 0x624cc0
  Thread  2 (LWP 13576) 

Mutex 0x624c70:
  Owner   9 (LWP 13583)  owns: this
  Thread  4 (LWP 13578)  owns: 0x624db0 0x624ec8

Mutex 0x624ea0:
  Owner  10 (LWP 13584)  owns: this 0x624cc0
  Thread  7 (LWP 13581)  owns: 0x624d88

Mutex 0x624c48:
  Owner   6 (LWP 13580)  owns: 0x624d10 this
  Thread  8 (LWP 13582) 

Threads not waiting for a mutex:
* Thread  5 (LWP 13579) 
  Thread  1 (LWP 13572) 

Detected deadlocks:
  10 (LWP 13584) -> 7 (LWP 13581) -> 10 (LWP 13584)
  4 (LWP 13578) -> 6 (LWP 13580) -> 9 (LWP 13583) -> 4 (LWP 13578)

```
