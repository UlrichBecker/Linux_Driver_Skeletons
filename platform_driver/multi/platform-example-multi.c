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


#define MAX_INSTANCES 5
#define NAME_LEN 32

struct MY_INSTANCE_T
{
    unsigned int myValue;
    unsigned int instanceNumber;
    struct miscdevice miscdev;
    char name[NAME_LEN];
};

struct GLOBAL_T
{
   unsigned int maxInstances;
   struct MY_INSTANCE_T* pMyInstances;
};

static struct GLOBAL_T global =
{
    .maxInstances = MAX_INSTANCES,
    .pMyInstances = NULL
};

/*-----------------------------------------------------------------------------
 */
static ssize_t onRead( struct file* pFile,       /*!< @see include/linux/fs.h   */
                       char __user* pUserBuffer, /*!< buffer to fill with data */
                       size_t userCapacity,      /*!< maximum size to copy     */
                       loff_t* pOffset )         /*!< pointer to the already copied bytes */
{
   DEBUG_MESSAGE( "minor: %d\n", ((struct miscdevice*)pFile->private_data)->minor );
   struct MY_INSTANCE_T* pMyInstances = container_of( pFile->private_data, struct MY_INSTANCE_T, miscdev );
   DEBUG_MESSAGE( "instance: %d, myValue: %d", pMyInstances->instanceNumber, pMyInstances->myValue );
   return 0;
}

/*-----------------------------------------------------------------------------
 */
static ssize_t onWrite( struct file *pFile,
                        const char __user* pUserBuffer,
                        size_t len,
                        loff_t* pOffset )
{
   DEBUG_MESSAGE( "minor: %d\n", ((struct miscdevice*)pFile->private_data)->minor );

   struct MY_INSTANCE_T* pMyInstances = container_of( pFile->private_data, struct MY_INSTANCE_T, miscdev );
   DEBUG_MESSAGE( "instance: %d, myValue: %d", pMyInstances->instanceNumber, pMyInstances->myValue );

   return 0;
}

/*-----------------------------------------------------------------------------
 */
static int onOpen( struct inode* pInode, struct file* pFile )
{  /*
    * Initializing of pFile->private_data is not necessary by misc-drivers
    * the kernel will initializing it with the pointer to struct miscdevice.
    */
    DEBUG_MESSAGE( ": Minor-number: %d\n", MINOR(pInode->i_rdev) );

    struct MY_INSTANCE_T* pMyInstances = container_of( pFile->private_data, struct MY_INSTANCE_T, miscdev );
    DEBUG_MESSAGE( "instance: %d, myValue: %d", pMyInstances->instanceNumber, pMyInstances->myValue );

    return 0;
}

/*-----------------------------------------------------------------------------
 */
static int onClose( struct inode *pInode, struct file* pFile )
{
   DEBUG_MESSAGE( ": Minor-number: %d\n", MINOR(pInode->i_rdev) );

   struct MY_INSTANCE_T* pMyInstances = container_of( pFile->private_data, struct MY_INSTANCE_T, miscdev );
   DEBUG_MESSAGE( "instance: %d, myValue: %d", pMyInstances->instanceNumber, pMyInstances->myValue );

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

    /*
     * Probing hardware here...
     */

    /*
     * The value of global.maxInstances could be obtained from a proberty
     * of the device-tree.
     */

    struct MY_INSTANCE_T* pMyInstances = devm_kzalloc( &pPdev->dev,
                                                sizeof(struct MY_INSTANCE_T) * global.maxInstances,
                                                GFP_KERNEL );
    if( pMyInstances == NULL )
    {
       ERROR_MESSAGE( "devm_kzalloc\n" );
       return -ENOMEM;
    }

    BUG_ON( sizeof(pMyInstances[0].name) <= strlen(DEVICE_BASE_FILE_NAME) + 2 );

    for( int i = 0; i < global.maxInstances; i++ )
    {
       pMyInstances[i].instanceNumber = i;
       pMyInstances[i].miscdev.minor = MISC_DYNAMIC_MINOR;
       snprintf( pMyInstances[i].name, sizeof(pMyInstances[0].name),
                 DEVICE_BASE_FILE_NAME "%d", i  );
       pMyInstances[i].miscdev.name = pMyInstances[i].name;
       pMyInstances[i].miscdev.fops = &mg_fops;
       pMyInstances[i].myValue = 4711 + i;
       if( misc_register( &pMyInstances[i].miscdev ) != 0 )
       {
          ERROR_MESSAGE( "misc_register\n" );
          for( int j = 0; j < i; j++ )
          {
             DEBUG_MESSAGE( "Removing: instance %d, minor: %d\n", j, pMyInstances[j].miscdev.minor );
             misc_deregister( &pMyInstances[j].miscdev );
          }
          return -ENODEV;
       }
       DEBUG_MESSAGE( "Instance: %d, minor: %d has been created\n", i, pMyInstances[i].miscdev.minor );
    }

    /*
     * Don't forget this! ;-)
     */
    platform_set_drvdata( pPdev, pMyInstances );

    return 0;
}

/*-----------------------------------------------------------------------------
 */
static int onRemove( struct platform_device *pPdev )
{
    DEBUG_MESSAGE( "%s\n", pPdev->name );
    struct MY_INSTANCE_T* pMyInstances = platform_get_drvdata( pPdev );
    for( int i = 0; i < global.maxInstances; i++ )
    {
       INFO_MESSAGE( "myValue: %d\n", pMyInstances[i].myValue );
       DEBUG_MESSAGE( "Removing instance: %d, minor: %d\n", i, pMyInstances[i].miscdev.minor );
       misc_deregister( &pMyInstances[i].miscdev );
    }
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
