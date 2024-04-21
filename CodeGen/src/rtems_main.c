/*  Init
 *
 *  This routine is the initialization task for this test program.
 *  It is called from init_exec and has the responsibility for creating
 *  and starting the tasks that make up the test.  If the time of day
 *  clock is required for the test, it should also be set to a known
 *  value by this function.
 *
 *  Input parameters:  NONE
 *
 *  Output parameters:  NONE
 *
 *  COPYRIGHT (c) 1989-1999.
 *  On-Line Applications Research Corporation (OAR).
 *
 *  The license and distribution terms for this file may be
 *  found in the file LICENSE in this distribution or at
 *  http://www.rtems.com/license/LICENSE.
 *
 *  $Id: init.c,v 1.12.4.1 2003/09/04 18:46:30 joel Exp $
 */

#define CONFIGURE_INIT

#include <rtems.h>

#define SHELL_TASK_PRIORITY 100
#define RT_TASK_PRIORITY 50

/* functions */

rtems_task Init(rtems_task_argument argument);

/* configuration information */

#include <bsp.h> /* for device driver prototypes */

#define CONFIGURE_APPLICATION_NEEDS_CONSOLE_DRIVER
#define CONFIGURE_APPLICATION_NEEDS_CLOCK_DRIVER
#define CONFIGURE_APPLICATION_NEEDS_LIBBLOCK

#define TICKS_PER_SECOND 1000

#define RTEMS_BSD_CONFIG_NET_PF_UNIX
#define RTEMS_BSD_CONFIG_NET_IP_MROUTE

#ifdef RTEMS_BSD_MODULE_NETINET6
#define RTEMS_BSD_CONFIG_NET_IP6_MROUTE
#endif

#define RTEMS_BSD_CONFIG_NET_IF_BRIDGE
#define RTEMS_BSD_CONFIG_NET_IF_LAGG
#define RTEMS_BSD_CONFIG_NET_IF_VLAN
#define RTEMS_BSD_CONFIG_BSP_CONFIG
#define RTEMS_BSD_CONFIG_INIT

#define CONFIGURE_MAXIMUM_TIMERS 64
#define CONFIGURE_MAXIMUM_MESSAGE_QUEUES 64
#define CONFIGURE_MAXIMUM_SEMAPHORES 64
#define CONFIGURE_MAXIMUM_TASKS 64
#define CONFIGURE_MAXIMUM_PERIODS 4
#define CONFIGURE_MAXIMUM_USER_EXTENSIONS 2
#define CONFIGURE_MAXIMUM_FILE_DESCRIPTORS 64
/*#define CONFIGURE_MAXIMUM_DRIVERS (CONFIGURE_NUMBER_OF_DRIVERS+10)*/
#define CONFIGURE_MAXIMUM_DRIVERS 32

#ifdef RTEMS_POSIX_API
#define CONFIGURE_MAXIMUM_POSIX_THREADS 40
#define CONFIGURE_MAXIMUM_POSIX_SEMAPHORES 20
#define CONFIGURE_MAXIMUM_POSIX_KEYS 4
#define CONFIGURE_MAXIMUM_POSIX_KEY_VALUE_PAIRS 8
#endif /*RTEMS_POSIX_API*/

#define CONFIGURE_RTEMS_INIT_TASKS_TABLE

#define CONFIGURE_APPLICATION_NEEDS_NULL_DRIVER
#define CONFIGURE_APPLICATION_NEEDS_ZERO_DRIVER

#define CONFIGURE_USE_IMFS_AS_BASE_FILESYSTEM
/*#define CONFIGURE_USE_MINIIMFS_AS_BASE_FILESYSTEM*/

#define CONFIGURE_MICROSECONDS_PER_TICK 1000

#define CONFIGURE_EXTRA_TASK_STACKS (10 * (RTEMS_MINIMUM_STACK_SIZE + 2 * 1024))
/*#define CONFIGURE_STACK_CHECKER_ENABLED 1*/

#define CONFIGURE_INIT_TASK_STACK_SIZE (10 * 1024)
#define CONFIGURE_INIT_TASK_PRIORITY 120
#define CONFIGURE_INIT_TASK_INITIAL_MODES                                      \
  (RTEMS_PREEMPT | RTEMS_NO_TIMESLICE | RTEMS_NO_ASR | RTEMS_INTERRUPT_LEVEL(0))

#include <rtems/confdefs.h>

/* end of include file */

#include <fcntl.h>
#include <rtems/error.h>
#include <rtems/monitor.h>
#include <rtems/shell.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>

#include <assert.h>

#ifdef APPL_WITH_BSDNET
#include <machine/rtems-bsd-config.h>
#include <rtems/dhcpcd.h>

#define DEFAULT_NETWORK_DHCPCD_ENABLE

#if defined(DEFAULT_NETWORK_DHCPCD_ENABLE) &&                                  \
    !defined(DEFAULT_NETWORK_NO_STATIC_IFCONFIG)
#define DEFAULT_NETWORK_NO_STATIC_IFCONFIG
#endif

#include <sysexits.h>

#include <rtems/bsd/bsd.h>
#include <rtems/rtems_bsdnet.h>

#endif /*CONFIG_APP_CAN_TEST_WITH_BSDNET*/

#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define CONFIGURE_SHELL_COMMANDS_INIT
#define CONFIGURE_SHELL_COMMANDS_ALL
#define CONFIGURE_SHELL_MOUNT_MSDOS

#include <bsp/irq-info.h>

#ifdef APPL_WITH_BSDNET

#include <rtems/netcmds-config.h>

#ifdef RTEMS_BSD_MODULE_USER_SPACE_WLANSTATS
#define SHELL_WLANSTATS_COMMAND &rtems_shell_WLANSTATS_Command,
#else
#define SHELL_WLANSTATS_COMMAND
#endif

#ifdef RTEMS_BSD_MODULE_USR_SBIN_WPA_SUPPLICANT
#define SHELL_WPA_SUPPLICANT_COMMAND &rtems_shell_WPA_SUPPLICANT_Command,
#else
#define SHELL_WPA_SUPPLICANT_COMMAND
#endif

#ifdef SHELL_TTCP_COMMAND_ENABLE
#define SHELL_TTCP_COMMAND &rtems_shell_TTCP_Command,
#else
#define SHELL_TTCP_COMMAND
#endif

#define CONFIGURE_SHELL_USER_COMMANDS                                          \
  SHELL_WLANSTATS_COMMAND                                                      \
  SHELL_WPA_SUPPLICANT_COMMAND                                                 \
  SHELL_TTCP_COMMAND                                                           \
  &bsp_interrupt_shell_command, &rtems_shell_ARP_Command,                      \
      &rtems_shell_HOSTNAME_Command, &rtems_shell_PING_Command,                \
      &rtems_shell_ROUTE_Command, &rtems_shell_NETSTAT_Command,                \
      &rtems_shell_IFCONFIG_Command, &rtems_shell_TCPDUMP_Command,             \
      &rtems_shell_SYSCTL_Command, &rtems_shell_VMSTAT_Command

#endif /*APPL_WITH_BSDNET*/

#include <rtems/shellconfig.h>

#define BUILD_VERSION_STRING(major, minor, patch)                              \
  __XSTRING(major) "." __XSTRING(minor) "." __XSTRING(patch)

void bad_rtems_status(rtems_status_code status, int fail_level,
                      const char *text) {
  printf("ERROR: %s status %s", text, rtems_status_text(status));
  status = rtems_task_delete(RTEMS_SELF);
}

int testcmd_forshell(int argc, char **argv) {
  int i;
  printf("Command %s called\n", argv[0]);
  for (i = 1; i < argc; i++)
    if (argv[i])
      printf("%s", argv[i]);
  printf("\n");
  return 0;
}

#ifdef APPL_WITH_BSDNET

static void default_network_dhcpcd(void) {
#ifdef DEFAULT_NETWORK_DHCPCD_ENABLE
  static const char default_cfg[] = "clientid libbsd test client\n";
  rtems_status_code sc;
  int fd;
  int rv;
  ssize_t n;

  fd =
      open("/etc/dhcpcd.conf", O_CREAT | O_WRONLY, S_IRWXU | S_IRWXG | S_IRWXO);
  assert(fd >= 0);

  n = write(fd, default_cfg, sizeof(default_cfg) - 1);
  assert(n == (ssize_t)sizeof(default_cfg) - 1);

#ifdef DEFAULT_NETWORK_DHCPCD_NO_DHCP_DISCOVERY
  static const char nodhcp_cfg[] = "nodhcp\nnodhcp6\n";

  n = write(fd, nodhcp_cfg, sizeof(nodhcp_cfg) - 1);
  assert(n == (ssize_t)sizeof(nodhcp_cfg) - 1);
#endif

  rv = close(fd);
  assert(rv == 0);

  sc = rtems_dhcpcd_start(NULL);
  assert(sc == RTEMS_SUCCESSFUL);
#endif
}

int network_init(void) {
  rtems_status_code sc;

  sc = rtems_bsd_initialize();
  if (sc != RTEMS_SUCCESSFUL) {
    printf("rtems_bsd_initialize failed\n");
    return -1;
  }

  /* Let the callout timer allocate its resources */

  sc = rtems_task_wake_after(2);
  if (sc != RTEMS_SUCCESSFUL) {
    printf("rtems_task_wake_after failed\n");
    return -1;
  }

  default_network_dhcpcd();

  return 0;
}

#endif /*APPL_WITH_BSDNET*/

#define XNAME(x,y)  x##y
#define NAME(x,y)   XNAME(x,y)
#define STR(x) #x

int NAME(MODEL,_init)(void);
int NAME(MODEL,_isr)(double);
int NAME(MODEL,_end)(void);
double NAME(MODEL,_get_tsamp)(void);

#define NSEC_PER_SEC    1000000000
#define USEC_PER_SEC    1000000

static volatile int end = 0;
static double T = 0.0;
static double Tsamp;

/* Options presettings */
static char rtversion[] = "0.9";
static int prio = 99;
static int verbose = 0;
static int wait = 0;
static int extclock = 0;
double FinalTime = 0.0;

double get_run_time(void)
{
  return(T);
}

double get_Tsamp(void)
{
  return(Tsamp);
}

int get_priority_for_com(void)
{
  if (prio < 0)
    {
      return -1;
    }
  else
    {
      return prio - 1;
    }
}

static inline void tsnorm(struct timespec *ts)
{
  while (ts->tv_nsec >= NSEC_PER_SEC) {
    ts->tv_nsec -= NSEC_PER_SEC;
    ts->tv_sec++;
  }
}

static inline double calcdiff(struct timespec t1, struct timespec t2)
{
  double diff;
  diff = 1.0 * ((long) t1.tv_sec - (long) t2.tv_sec);
  diff += 1e-9*t1.tv_nsec - 1e-9*t2.tv_nsec;
  return (diff);
}

static void *rt_task(void *p)
{
  struct timespec t_next, t_current, t_isr, T0;
  struct sched_param param;

  param.sched_priority = prio;
//  if(sched_setscheduler(0, SCHED_FIFO, &param)==-1){
//    perror("sched_setscheduler failed");
//    exit(-1);
//  }

#ifdef HAVE_MLOCK
  mlockall(MCL_CURRENT | MCL_FUTURE);
#endif

  Tsamp = NAME(MODEL,_get_tsamp)();

  t_isr.tv_sec =  0L;
  t_isr.tv_nsec = (long)(1e9*Tsamp);
  tsnorm(&t_isr);

  T=0;

  NAME(MODEL,_init)();

#ifdef CANOPEN
  canopen_synch();
#endif

  /* get current time */
  clock_gettime(CLOCK_MONOTONIC,&t_current);
  T0 = t_current;

  while(!end){

    /* periodic task */
    T = calcdiff(t_current,T0);

    NAME(MODEL,_isr)(T);

#ifdef CANOPEN
    canopen_synch();
#endif

    if((FinalTime >0) && (T >= FinalTime)) break;

    t_next.tv_sec = t_current.tv_sec + t_isr.tv_sec;
    t_next.tv_nsec = t_current.tv_nsec + t_isr.tv_nsec;
    tsnorm(&t_next);

    /* Check if Overrun */
    clock_gettime(CLOCK_MONOTONIC,&t_current);
    if (t_current.tv_sec > t_next.tv_sec ||
	(t_current.tv_sec == t_next.tv_sec && t_current.tv_nsec > t_next.tv_nsec)) {
      int usec = (t_current.tv_sec - t_next.tv_sec) * 1000000 + (t_current.tv_nsec -
								 t_next.tv_nsec)/1000;
      fprintf(stderr, "Base rate overrun by %d us\n", usec);
      t_next= t_current;
    }
    clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &t_next, NULL);
    t_current = t_next;
  }
  NAME(MODEL,_end)();
  pthread_exit(0);
}

void endme(int n)
{
  end = 1;
}

rtems_id   rt_task_id;
rtems_name rt_task_name;

rtems_task Init(rtems_task_argument ignored) {
  rtems_status_code status;
  printf("\n\nRTEMS v " BUILD_VERSION_STRING(__RTEMS_MAJOR__, __RTEMS_MINOR__,
                                             __RTEMS_REVISION__) "\n");

#ifdef APPL_WITH_BSDNET
  if (network_init() == -1) {
    printf("rtems_bsd_ifconfig_lo0 failed\n");
  };
#endif /*APPL_WITH_BSDNET*/

  rtems_monitor_init(RTEMS_MONITOR_SUSPEND | RTEMS_MONITOR_GLOBAL);
  /*rtems_capture_cli_init (0);*/

  printf("Starting application pysimCoder generated" STR(MODEL) "\n");

  rtems_shell_init("SHLL", RTEMS_MINIMUM_STACK_SIZE + 0x1000,
                   SHELL_TASK_PRIORITY, "/dev/console", 1, 0, NULL);

  //rtems_shell_add_cmd("zynq_apfoo", "app", "zynq_apfoo", zynq_apfoo);

  rt_task_name = rtems_build_name( 'R', 'T', '_', 'T' );

  status = rtems_task_create(
     rt_task_name,
     RT_TASK_PRIORITY,
     RTEMS_MINIMUM_STACK_SIZE+0x10000,
     RTEMS_DEFAULT_MODES /*& ~(RTEMS_TIMESLICE_MASK) | RTEMS_TIMESLICE*/,
     RTEMS_DEFAULT_ATTRIBUTES,
     &rt_task_id
  );
  if(!rtems_is_status_successful(status)) {
    printf("rt_task allocation failed\n");
  }

  status = rtems_task_start( rt_task_id, rt_task, 0 );
  if(!rtems_is_status_successful(status)) {
    printf("rt_task start failed\n");
  }

  rtems_task_delete(RTEMS_SELF);

  printf("*** END OF INIT TASK ***\n");
  exit(0);
}
