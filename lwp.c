#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <rr.h>
#include <lwp.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/resource.h>

static tid_t counter = 0;	// number of threads. tids are one-index, so == tid of last thread
static thread first = NULL;	// head of lwp global list. used by create()
static thread current = NULL;   // running thread. used by yield() etc
static size_t stk_size = 0;

/* lwp functions */
static void lwp_wrap( lwpfun func, void* arg ) {
    int rval = func( arg );
    lwp_exit( rval );
}

extern tid_t lwp_create( lwpfun func, void* arg ) {
    thread new = malloc( sizeof( context ) );

    counter++;
    new->tid = counter;
    
    if ( !stk_size ) {
	// look up page size with sysconf() _SC_PAGE_SIZE
	long ps = sysconf( _SC_PAGE_SIZE );
	// look up resource limit with getrlimit() RLIMIT_STACK
	struct rlimit rl;
	getrlimit( RLIMIT_STACK, &rl );
	// choose stack size to mmap as smallest multiple of page size geq soft limit
	stk_size = (size_t) ( ( (long) rl.rlim_cur - 1 )/ps + 1)*ps;
    }
    new->stacksize = stk_size;
    
    unsigned long* stk = mmap( NULL, stk_size, PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS|MAP_STACK, -1, 0);
    new->stack = stk;
    int end = stk_size/sizeof(unsigned long) - 1;
    stk[end] = 0;					// stk[0]-stk[1] is ra, fp of lwp_wrap fake call
    stk[end - 1] = 0;
    stk[end - 2] = (unsigned long) lwp_wrap;		// stk[2] is ra of swap_rfiles into lwp_wrap
    stk[end - 3] = (unsigned long) (stk + end - 1);	// stk[3] is fp of swap_rfiles, points to fake call

    swap_rfiles( &(new->state), NULL );
    new->state.rdi = (unsigned long) func;		// set args as argument to lwp_wrap
    new->state.rsi = (unsigned long) arg;
    new->state.rbp = (unsigned long) (stk + end - 3);	// set sp and fp registers to point at fp
    new->state.rsp = (unsigned long) (stk + end - 3);
    new->state.fxsave = FPU_INIT;

    new->status = LWP_LIVE;

    // insert self into lwp global list
    if ( !first ) {
	first = new;
	new->lib_one = new;
	new->lib_two = new;
    } else {
	new->lib_one = first;		// next thread in list
	new->lib_two = first->lib_two;	// prev thread in list
	first->lib_two = new;
	new->lib_two->lib_one = new;
    }

    RoundRobin->admit( new );

    return( new->tid );
}

extern void lwp_exit( int status ) {
    thread cur = current;
    RoundRobin->remove( cur );
    cur->status = MKTERMSTAT( LWP_TERM, status );
    thread next = RoundRobin->next();
    current = next;
    // fprintf( stderr, "thread %i exiting.\n", cur->tid );
    if ( current ) swap_rfiles( &(cur->state), &(next->state) );
}

extern tid_t lwp_gettid( void ) { return( current->tid ); }

extern void lwp_yield( void ) {
    thread cur = current;
    thread next = RoundRobin->next();
    current = next;
    swap_rfiles( &(cur->state), &(next->state) );
}

extern void lwp_start( void ) {
    thread parent = malloc( sizeof( context ) );

    counter++;
    parent->tid = counter;
    parent->stacksize = 0;
    parent->stack = NULL;
    parent->status = LWP_LIVE;
    
    if ( !first ) {
	first = parent;
	parent->lib_one = parent;
	parent->lib_two = parent;
    } else {
	parent->lib_one = first;
	parent->lib_two = first->lib_two;
	first->lib_two = parent;
	parent->lib_two->lib_one = parent;
    }

    RoundRobin->admit( parent );
    current = parent;

    lwp_yield();
}

static thread lwp_get_exited( void ) {	// returns pointer to first thread in list with TERM status
    // fprintf( stderr, "--  lwp: parent checking for terminated threads:\n" );
    if ( LWPTERMINATED( current->status ) ) {
	fprintf( stderr, "--  lwp: error - current thread marked as terminated.\n" );
	exit( 1 );
    }
    thread check = current->lib_one;
    while ( !LWPTERMINATED( check->status ) ) {
	// fprintf( stderr, "--  lwp:   thread %i not terminated.\n", check->tid );
	if ( check == current ) return( NULL );
	check = check->lib_one;
    }
    // fprintf( stderr, "--  lwp:   thread %i terminated.\n", check->tid );
    return( check );
}

extern tid_t lwp_wait( int* status ) {
    if ( RoundRobin->qlen() < 2 ) return( NO_THREAD );
    thread caught = lwp_get_exited();
    while ( !caught ) {
	lwp_yield();
	caught = lwp_get_exited();
    }
    tid_t ret = caught->tid;
    // fprintf( stderr, "--  lwp: thread %i caught.\n", ret );
    if ( status ) *status = LWPTERMSTAT( caught->status );
    caught->lib_one->lib_two = caught->lib_two;
    caught->lib_two->lib_one = caught->lib_one;
    // fprintf( stderr, "--  lwp:   list rearranged.\n" );
    munmap( caught->stack, stk_size );
    // fprintf( stderr, "--  lwp:   stack unmapped.\n" );
    free( caught );
    // fprintf( stderr, "--  lwp: thread %i caught and freed.\n", ret );
    return( ret );
}

extern void lwp_set_scheduler( scheduler fun ) {}
extern scheduler lwp_get_scheduler( void ) { return( RoundRobin ); }

extern thread tid2thread( tid_t tid ) {
    if ( counter == 0 || tid > counter ) return( NULL );
    thread cur = first;
    if ( tid < counter/2 ) {
	while ( cur->tid < tid ) cur = cur->lib_one;
    } else {
	do cur = cur->lib_two; while ( cur->tid > tid );
    }
    if ( cur->tid != tid ) return( NULL );
    return cur;
}
