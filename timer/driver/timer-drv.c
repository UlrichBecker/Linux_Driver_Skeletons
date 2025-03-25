/*****************************************************************************/
/*                                                                           */
/*! @brief Demonstration of the timer-function running in the linux-kernel   */
/*                                                                           */
/*---------------------------------------------------------------------------*/
/*! @file    timer-drv.c                                                     */
/*! @author  Ulrich Becker                                                   */
/*! @date    25.03.2025                                                      */
/*****************************************************************************/
#include <linux/version.h>
#include <linux/printk.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/timer.h>

MODULE_LICENSE( "GPL" );


#define CONFIG_DEBUG_SKELETON
#define DEVICE_BASE_FILE_NAME KBUILD_MODNAME

/* Begin of message helper macros for "dmesg" *********************************/
/* NOTE for newer systems with "systend"
 * "dmesg -w" corresponds the old “tail -f /var/log/messages”
 */
#define ERROR_MESSAGE( constStr, n... ) \
   printk( KERN_ERR DEVICE_BASE_FILE_NAME "-systemerror %d: %s" constStr, __LINE__, __func__, ## n )

#if defined( CONFIG_DEBUG_SKELETON ) || defined(__DOXYGEN__)
   #define DEBUG_MESSAGE( constStr, n... ) \
      printk( KERN_DEBUG DEVICE_BASE_FILE_NAME "-dbg %d: %s" constStr, __LINE__, __func__, ## n )

   #define DEBUG_ACCESSMODE( pFile ) \
      DEBUG_MESSAGE( ": access: %s\n", \
                     (pFile->f_flags & O_NONBLOCK)? "non blocking":"blocking" )
#else
   #define DEBUG_MESSAGE( constStr, n... )
   #define DEBUG_ACCESSMODE( pFile )
#endif

#define INFO_MESSAGE( constStr, n... ) \
   printk( KERN_INFO DEVICE_BASE_FILE_NAME ": " constStr, ## n )
/* End of message helper macros for "dmesg" ++++++++***************************/

/*!
 * @brief Creating a timer object with the ability
 *        to transport additional private data.
 */
struct MY_TIMER_T
{
    struct timer_list timer;
    void*  pPrivate;
};

static struct MY_TIMER_T g_timer;

#define PERIOD_MS 1000

/*!----------------------------------------------------------------------------
 * @brief Callback function of the timer
 */
void onMyTimer( struct timer_list* pTimer )
{
    struct MY_TIMER_T* pMyTimer = from_timer( pMyTimer, pTimer, timer );
    INFO_MESSAGE( "onMyTimer() private data: %d\n", (int)pMyTimer->pPrivate );

    /*
     * Repeat the timer
     */
    mod_timer( pTimer, jiffies + msecs_to_jiffies(PERIOD_MS) );
}

/*!----------------------------------------------------------------------------
 * @brief Driver constructor
 */
static int __init driverInit( void )
{
    DEBUG_MESSAGE("\n");

    /*
     * Initializing the timer object
     */
    timer_setup( &g_timer.timer, onMyTimer, 0 );
    g_timer.pPrivate = (void*) 4711;

    /*
     * Start the timer
     */
    mod_timer( &g_timer.timer, jiffies + msecs_to_jiffies(PERIOD_MS) );
    return 0;
}

/*!----------------------------------------------------------------------------
 * @brief Driver destructor
 */
static void __exit driverExit( void )
{
    DEBUG_MESSAGE("\n");
    del_timer_sync( &g_timer.timer );
}

module_init( driverInit );
module_exit( driverExit );
/*================================== EOF ====================================*/
