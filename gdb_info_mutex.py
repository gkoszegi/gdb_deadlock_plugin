import gdb

class ThreadHolder:
    """A class that can be used with the 'with' statement to save and
    restore the current thread while operating on some other thread."""

    def __init__(self, thread):
        self.thread = thread

    def __enter__(self):
        self.save = gdb.selected_thread()
        self.thread.switch()

    def __exit__ (self, exc_type, exc_value, traceback):
        try:
            self.save.switch()
        except:
            pass
        return None

def print_thread(thr, mut, owns=[]):
    "A helper function to nicely print a gdb.Thread."
    (pid, lwp, tid) = thr.ptid
    out = "* " if thr == gdb.selected_thread() else "  "
    out += "Owner  " if mut in owns else "Thread "
    out += "%2d (LWP %d) " % (thr.num, lwp)
    if owns:
        out += " owns: %s" % (" ".join(map(lambda addr: ("0x%x" % addr) if addr != mut else "this", owns)))
    print(out)

def to_num(data):
    try:
        return int(data) # works on gdb-7.7.1 with python 3.4.0
    except:
        return long(data) # works on gdb-7.4.1 with python 2.7.3

class InfoMutex(gdb.Command):
    def __init__ (self):
        gdb.Command.__init__ (self, "info mutex", gdb.COMMAND_NONE)
        self.lock_calls = ['__pthread_mutex_lock', '__pthread_mutex_lock_full',
                           'pthread_mutex_timedlock', '__GI___pthread_mutex_lock']

    def invoke (self, arg, from_tty):
        # Map a mutex ID to the LWP owning the mutex.
        owner = {}
        # Map an LWP id to a thread object.
        threads = {}
        # Map a mutex ID to a list of thread objects that are waiting
        # for the lock.
        mutexes = {}
        # Map LWP to a set of owned mutexes
        owns = {}
        # Map LWP to set of blocked LWPs
        blocked = {}

        for inf in gdb.inferiors():
            for thr in inf.threads():
                id = thr.ptid[1]
                threads[id] = thr
                with ThreadHolder (thr):
                    frame = gdb.selected_frame()
                    lock_name = None
                    owner_lwp = None
                    for n in range(5):
                        if frame is None:
                            break
                        fn_sym = frame.function()
                        if fn_sym is not None and fn_sym.name in self.lock_calls:
                            m = frame.read_var('mutex')
                            olwp = to_num(m['__data']['__owner'])
                            if olwp == 0:
                                # mutex is not locked, core was accidentally generated while this thread was locking it
                                break
                            lock_name = to_num(m)
                            owner_lwp = olwp
                            if lock_name not in owner:
                                owner[lock_name] = owner_lwp
                                if owner_lwp in owns:
                                    owns[owner_lwp].add(lock_name)
                                else:
                                    owns[owner_lwp] = set([lock_name])
                                break
                        frame = frame.older()

                    if lock_name not in mutexes:
                        mutexes[lock_name] = []
                    mutexes[lock_name] += [thr]

                    if owner_lwp not in blocked:
                        blocked[owner_lwp] = set([id])
                    else:
                        blocked[owner_lwp].add(id)

        for id in mutexes:
            if id is None:
                continue
            print("Mutex 0x%x:" % id)
            thr = threads[owner[id]]
            print_thread(thr, id, owns[thr.ptid[1]] if thr.ptid[1] in owns else [])
            for thr in mutexes[id]:
                print_thread(thr, id, owns[thr.ptid[1]] if thr.ptid[1] in owns else [])
            print('')

        if None in mutexes:
            print("Threads not waiting for a mutex:")
            for thr in mutexes[None]:
                print_thread(thr, None, owns[thr.ptid[1]] if thr.ptid[1] in owns else [])
            print('')

        def walk_chain(lwp, deadlocks, current):
            if lwp not in blocked:
                return
            current.append(lwp)
            for blocked_lwp in blocked[lwp]:
                if blocked_lwp in current:
                    deadlocks.append(current + [blocked_lwp])
                else:
                    fork = current[:]
                    walk_chain(blocked_lwp, deadlocks, fork)

        print("Detected deadlocks:")
        if blocked:
            deadlocks = []
            for lwp in blocked:
                walk_chain(lwp, deadlocks, [])
            unique = []
            for chain in deadlocks:
                deadset = set(chain)
                if deadset in unique:
                    continue
                unique.append(deadset)
                print("  %s" % (" -> ".join(map(lambda lwp: "%d (LWP %d)" % (threads[lwp].num, lwp), chain))))
        else:
            print("None")
        print('')

InfoMutex()
