#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <syslog.h>
#include <string.h>
#include <assert.h>
#include <signal.h>
#include <time.h>
#include "httpd.h"

#define BUFF_SIZE          1024
#define DEFAULT_PID        "/var/run/meteod.pid"
#define DAEMON_NAME        "meteod"
#define DELAY              30
int isrunned=1;

char pid_file[256], serial[16], file_path[100];
int delay=DELAY;

double ReadTemperature() {
    char buffer[BUFF_SIZE];
    FILE* ds18b20f;
    char* tptr;
    double temp=-127.666;
    sprintf(file_path, "/sys/bus/w1/devices/%s/w1_slave", serial);
    ds18b20f = fopen(file_path,"rt");
    if(ds18b20f== NULL) {
       syslog(LOG_ERR, "%s is unable to open w1_slave temps file",DAEMON_NAME);
    }
    else {
       if(fgets(buffer,sizeof(buffer),ds18b20f)) {
           if(strstr(buffer, "YES") == NULL) {
              syslog(LOG_WARNING, "CRC of a read failed!");
	   }
	   else {
              if(fgets(buffer,sizeof(buffer),ds18b20f)) {
		 if((tptr = strstr(buffer, "t=")) != NULL) {
                    int t = atoi(tptr +2);
                    temp=t/1000.0;
	         }
              }
              else {
                  syslog(LOG_WARNING, "Cannot find temperature in w1_slave file!");
              }
           }
	}
        else { 
           syslog(LOG_WARNING, "i can not read temerature of DS18B20 thermometer!");
        }
        fclose(ds18b20f);
    }
    return temp;
}

int ReadSerial(){
    FILE* slavesf;
    slavesf = fopen("/sys/bus/w1/devices/w1_bus_master1/w1_master_slaves","rt");
    if(slavesf == NULL) {
       syslog(LOG_ERR, "%s is unable to open w1_master_slaves file",DAEMON_NAME);
       return -1;
    }
    else {
       if(fgets(serial,sizeof(serial),slavesf)) {
           if(strncmp(serial,"28-", 3)) {
	   	syslog(LOG_WARNING, "i can only support DS18B20!");
	        fclose(slavesf);
                return -2;
	   }
	}
        else { 
           syslog(LOG_WARNING, "i can not find any DS18B20 thermometer!");
	   fclose(slavesf);
           return -3;
        }
        fclose(slavesf);
        return 0;
    }

}

void PrintUsage(int argc, char *argv[]) {
    if (argc >=1) {
        printf("Usage: %s -h -n\n", argv[0]);
        printf("  Options:\n");
        printf("      -n\tDon't fork off as a daemon.\n");
        printf("      -h\tShow this help screen.\n");
        printf("\n");
    }
}

/**************************************************************************
    Function: signal_handler
 
    Description:
        This function handles select signals that the daemon may
        receive.  This gives the daemon a chance to properly shut
        down in emergency situations.  This function is installed
        as a signal handler in the 'main()' function.
 
    Params:
        @sig - The signal received
 
    Returns:
        returns void always
**************************************************************************/
void signal_handler(int sig) {
 
    switch(sig) {
        case SIGTERM:
            syslog(LOG_WARNING, "Received SIGTERM signal, exiting...");
			isrunned=0;
            break;
        default:
            syslog(LOG_WARNING, "Unhandled signal (%d) %s", strsignal(sig));
            break;
    }
}
 
int main(int argc, char *argv[]) {

#if defined(DEBUG)
    int daemonize = 0;
#else
    int daemonize = 1;
#endif
    struct MHD_Daemon *mhd_daemon;

    // Setup signal handling before we start
    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);
    signal(SIGQUIT, signal_handler);
 
    strncpy(pid_file,DEFAULT_PID,sizeof(pid_file));
    int c;
    while( (c = getopt(argc, argv, "p:nh|help")) != -1) {
        switch(c){
            case 'h':
                PrintUsage(argc, argv);
                exit(0);
                break;
            case 'n':
                daemonize = 0;
                break;
			case 'p':
				strncpy(pid_file,optarg,sizeof(pid_file));
				break;
			case '?':
				if (optopt == 'p' )
					fprintf (stderr, "Option -%c requires an argument.\n", optopt);			
				else if (isprint (optopt))
					fprintf (stderr, "Unknown option `-%c'.\n", optopt);
				else
					fprintf (stderr, "Unknown option character `\\x%x'.\n", optopt);
				exit(EXIT_FAILURE);
            default:
                PrintUsage(argc, argv);
                exit(0);
                break;
        }
    }
 
    syslog(LOG_INFO, "%s daemon starting up", DAEMON_NAME);
   
    // Setup syslog logging - see SETLOGMASK(3)
#if defined(DEBUG)
    setlogmask(LOG_UPTO(LOG_DEBUG));
    openlog(DAEMON_NAME, LOG_CONS | LOG_NDELAY | LOG_PERROR | LOG_PID, LOG_USER);
#else
    setlogmask(LOG_UPTO(LOG_INFO));
    openlog(DAEMON_NAME, LOG_CONS, LOG_USER);
#endif
    
    /* Our process ID and Session ID */
    pid_t pid, sid;
	
    if (daemonize) {
        syslog(LOG_INFO, "starting the daemonizing process");
 
        /* Fork off the parent process */
        pid = fork();
        if (pid < 0) {
            exit(EXIT_FAILURE);
        }
        /* If we got a good PID, then
           we can exit the parent process. */
        if (pid > 0) {
			FILE* fpid = fopen(pid_file,"wt");
			if(fpid) {
				fprintf(fpid, "%d", pid);
				fclose(fpid);
			}
			else {
				syslog(LOG_WARNING, "unable to open pid file for writing");
			}
            exit(EXIT_SUCCESS);
        }
 
        /* Change the file mode mask */
        umask(0);
 
        /* Create a new SID for the child process */
        sid = setsid();
        if (sid < 0) {
            /* Log the failure */
            exit(EXIT_FAILURE);
        }
 
        /* Change the current working directory */
        if ((chdir("/")) < 0) {
            /* Log the failure */
            exit(EXIT_FAILURE);
        }
 
        /* Close out the standard file descriptors */
#if !defined(DEBUG)
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
#endif
    }
    if(ReadSerial() != 0) {
        exit(EXIT_FAILURE);
    }
    mhd_daemon = MHD_start_daemon (MHD_USE_SELECT_INTERNALLY, HTTPDPORT, NULL, NULL,
                             &answer_to_connection, NULL, MHD_OPTION_END);
	
    do {
	setTemperature(ReadTemperature());
        sleep(delay);
    }while(isrunned);
	
    syslog(LOG_INFO, "%s daemon exiting...", DAEMON_NAME);
	
    MHD_stop_daemon (mhd_daemon);
    exit(0);
}
