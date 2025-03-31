#include <linux/version.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>

MODULE_LICENSE( "GPL" );
#define CONFIG_DEBUG_SKELETON
#define DEVICE_BASE_FILE_NAME KBUILD_MODNAME

/* Begin of message helper macros for "dmesg" *********************************/
/* NOTE for newer systems with "systend"
 * "dmesg -w" corresponds the old “tail -f /var/log/messages”
 */
#define ERROR_MESSAGE( constStr, n... ) \
   printk( KERN_ERR DEVICE_BASE_FILE_NAME "-systemerror %d: %s " constStr, __LINE__, __func__, ## n )

#if defined( CONFIG_DEBUG_SKELETON ) || defined(__DOXYGEN__)
   #define DEBUG_MESSAGE( constStr, n... ) \
      printk( KERN_DEBUG DEVICE_BASE_FILE_NAME "-dbg %d: %s " constStr, __LINE__, __func__, ## n )

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
/*-----------------------------------------------------------------------------
 */
static int onProbe( struct platform_device* pPdev )
{
    DEBUG_MESSAGE( "%s\n", pPdev->name );
    return 0;
}

/*-----------------------------------------------------------------------------
 */
static int onRemove( struct platform_device *pPdev )
{
    DEBUG_MESSAGE( "%s\n", pPdev->name );
    return 0;
}

/*-----------------------------------------------------------------------------
 */
static void onShutdown( struct platform_device *pPdev )
{
    DEBUG_MESSAGE( "%s\n", pPdev->name );
}

/*-----------------------------------------------------------------------------
 */
int onSuspend( struct platform_device* pPdev, pm_message_t state )
{
    DEBUG_MESSAGE( "%s\n", pPdev->name );
    return 0;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 0, 0)
/*-----------------------------------------------------------------------------
 */
int onSuspendLate( struct platform_device* pPdev, pm_message_t state )
{
    DEBUG_MESSAGE( "%s\n", pPdev->name );
    return 0;
}

/*-----------------------------------------------------------------------------
 */
int onResumeEarly( struct platform_device* pPdev, pm_message_t state )
{
    DEBUG_MESSAGE( "%s\n", pPdev->name );
    return 0;
}
#endif

/*-----------------------------------------------------------------------------
 */
int onResume( struct platform_device* pPdev )
{
    DEBUG_MESSAGE( "%s\n", pPdev->name );
    return 0;
}

/*===========================================================================*/

static const struct of_device_id g_ofMatchList[] =
{
    { .compatible = "my,driver" },
    { .compatible = "i2c-gpio" },
    { }
};

MODULE_DEVICE_TABLE( of, g_ofMatchList );

static struct platform_driver myPlatformDriver=
{
    .probe        = onProbe,
    .remove       = onRemove,
    .shutdown     = onShutdown,
    .suspend      = onSuspend,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 0, 0)
    .suspend_late = onSuspendLate,
    .resume_early = onResumeEarly,
#endif
    .resume       = onResume,
    .driver =
    {
        .name = KBUILD_MODNAME,
        .owner = THIS_MODULE,
        .of_match_table = of_match_ptr( g_ofMatchList )
    },
};

module_platform_driver( myPlatformDriver );
/*================================== EOF ====================================*/
