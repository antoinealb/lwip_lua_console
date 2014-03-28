#include <stdio.h>

#include <lwip/sys.h>
#include <lwip/tcpip.h>
#include <lwip/ip.h>

#include <netif/tapif.h>

/** Shared semaphore to signal when lwIP init is done. */
sys_sem_t lwip_init_done;

/** Global var to say if we are client of server. */
int is_server = 0;

/** Serial net interface */
struct netif slipf;

void list_netifs(void)
{
    struct netif *n; /* used for iteration. */
    for (n = netif_list; n != NULL; n = n->next) {
        /* Converts the IP adress to a human readable format. */
        char buf[16+1];
        ipaddr_ntoa_r(&n->ip_addr, buf, 17);
        printf("%s: %s\n", n->name, buf);
    }
}

/** @rief Callback for lwIP init completion.
 *
 * This callback is automatically called from the lwIP thread after the
 * initialization is complete. It must then tell the main init task that it
 * can proceed. To do thism we use a semaphore that is posted from the lwIP
 * thread and on which the main init task is pending. */
void ipinit_done_cb(void *a)
{
    sys_sem_signal(&lwip_init_done);
}


/** @brief Inits the IP stack and the network interfaces.
 *
 * This function is responsible for the following :
 * 1. Initialize the lwIP library.
 * 2. Wait for lwIP init to be complete.
 * 3. Create the SLIP interface and give it a static adress/netmask.
 * 4. Set the SLIP interface as default and create a gateway.
 * 5. List all network interfaces and their settings, for debug purposes.
 */
void ip_stack_init(void) {
    /* Netif configuration */
    static ip_addr_t ipaddr, netmask, gw;

    IP4_ADDR(&gw, 10,0,0,1);
    IP4_ADDR(&ipaddr, 10,0,0,2);
    IP4_ADDR(&netmask, 255,255,255,0);

    /* Creates the "Init done" semaphore. */
    sys_sem_new(&lwip_init_done, 0);

    /* We start the init of the IP stack. */
    tcpip_init(ipinit_done_cb, NULL);

    /* We wait for the IP stack to be fully initialized. */
    printf("Waiting for LWIP init...\n");
    sys_sem_wait(&lwip_init_done);

    /* Deletes the init done semaphore. */
    sys_sem_free(&lwip_init_done);
    printf("LWIP init complete\n");

    /* Adds a tap pseudo interface for unix debugging. */
    netif_add(&slipf, &ipaddr, &netmask, &gw, &is_server, tapif_init, tcpip_input);

    netif_set_default(&slipf);
    netif_set_up(&slipf);

}




/** @brief Init task.
 *
 * This task is reponsible to initialize the whole system and to create tasks
 * for other services to run. We do it in a task because we need some of UC/OS-II
 * functions that are only available after OSStart().
 */
void init_task(void *pdata)
{
    ip_stack_init();

    /* Lists every network interface and shows its IP. */
    printf("Listing network interfaces...\n");
    list_netifs();
    luaconsole_init();
}

int main(int argc, char **argv)
{
    printf("==== Boot ====\n");



    init_task(NULL);

    /* We should never get here because OSSStart() never returns. */
    for(;;);
    return 0;
}
