#include <linux/version.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/io.h>
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


struct MY_DEVICE
{
    unsigned int myValue;
    struct miscdevice miscdev;
};

static ssize_t onRead( struct file* pFile,   /*!< @see include/linux/fs.h   */
                       char __user* pUserBuffer, /*!< buffer to fill with data */
                       size_t userCapacity,      /*!< maximum size to copy     */
                       loff_t* pOffset )         /*!< pointer to the already copied bytes */
{
    return 0;
}

/*-----------------------------------------------------------------------------
 */
static ssize_t onWrite( struct file *pFile,
                        const char __user* pUserBuffer,
                        size_t len,
                        loff_t* pOffset )
{
   return 0;
}

/*-----------------------------------------------------------------------------
 */
static int onOpen( struct inode* pInode, struct file* pFile )
{
    DEBUG_MESSAGE( ": Minor-number: %d\n", MINOR(pInode->i_rdev) );
    return 0;
}

/*-----------------------------------------------------------------------------
 */
static int onClose( struct inode *pInode, struct file* pFile )
{
   DEBUG_MESSAGE( ": Minor-number: %d\n", MINOR(pInode->i_rdev) );
   return 0;
}

static struct file_operations mg_fops =
{
  .owner          = THIS_MODULE,
  .open           = onOpen,
  .release        = onClose,
  .read           = onRead,
  .write          = onWrite,
};

/*-----------------------------------------------------------------------------
 */
static int onProbe( struct platform_device* pPdev )
{
    DEBUG_MESSAGE( "%s\n", pPdev->name );

    struct MY_DEVICE* pMyDevice = devm_kzalloc( &pPdev->dev, sizeof(struct MY_DEVICE), GFP_KERNEL );
    if( pMyDevice == NULL )
    {
       ERROR_MESSAGE( "devm_kzalloc\n" );
       return -ENOMEM;
    }

    pMyDevice->miscdev.minor = MISC_DYNAMIC_MINOR;
    pMyDevice->miscdev.name = DEVICE_BASE_FILE_NAME;
    pMyDevice->miscdev.fops = &mg_fops;
    pMyDevice->myValue = 4711;
    if( misc_register( &pMyDevice->miscdev ) != 0 )
    {
       ERROR_MESSAGE( "misc_register\n" );
       return -ENODEV;
    }
    platform_set_drvdata( pPdev, pMyDevice );
    DEBUG_MESSAGE( "minor: %d\n", pMyDevice->miscdev.minor );
    return 0;
}

/*-----------------------------------------------------------------------------
 */
static int onRemove( struct platform_device *pPdev )
{
    DEBUG_MESSAGE( "%s\n", pPdev->name );
    struct MY_DEVICE* pMyDevice = platform_get_drvdata( pPdev );
    INFO_MESSAGE( "myValue: %d\n", pMyDevice->myValue );
    misc_deregister( &pMyDevice->miscdev );
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
    { .compatible = "gsi-eps,timer_irq" },
    {}
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
