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
#include <linux/poll.h>

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


#define PERIOD_MS 1000

typedef struct
{
   unsigned int      minor;
   struct timer_list timer;
   unsigned int      period;
 //  atomic_t          openCount;
   struct mutex      oMutex;
   wait_queue_head_t readWaitQueue;
 //  wait_queue_head_t writeWaitQueue;
 //  int               index;
 //  char              buffer[16];
} INSTANCE_T;

#define MAX_INSTANCES 2

typedef struct
{
   dev_t                   deviceNumber;
   struct cdev*            pObject;
   struct class*           pClass;
   INSTANCE_T              instance[MAX_INSTANCES];
} MODULE_GLOBAL_T;

static MODULE_GLOBAL_T mg;


/*!----------------------------------------------------------------------------
 * @brief
 */
static int onOpen( struct inode* pInode, struct file* pFile )
{
   unsigned int instanceIndex = MINOR(pInode->i_rdev);
   INSTANCE_T* pObject = &mg.instance[ instanceIndex ];
   pObject->minor = instanceIndex;

   DEBUG_MESSAGE( ": Minor-number: %d\n", instanceIndex );
   BUG_ON( pFile->private_data != NULL );
   BUG_ON( instanceIndex >= MAX_INSTANCES );

   pFile->private_data = pObject;
   return 0;
}

/*!----------------------------------------------------------------------------
 * @brief
 */
static unsigned int onPoll( struct file* pFile, poll_table* pPollTable )
{
   unsigned int ret = 0;
   INSTANCE_T* pInstance = (INSTANCE_T*)pFile->private_data;
   DEBUG_MESSAGE( ": Minor-number: %d\n", ((INSTANCE_T*)pFile->private_data)->minor );

   mutex_lock( &pInstance->oMutex );
   poll_wait( pFile, &pInstance->readWaitQueue, pPollTable );
   ret |= (POLLIN | POLLRDNORM);
   mutex_unlock( &pInstance->oMutex );

   return ret;
}

/*!----------------------------------------------------------------------------
 * @brief
 */
static ssize_t onRead( struct file* pFile,   /*!< @see include/linux/fs.h   */
                       char __user* pUserBuffer, /*!< buffer to fill with data */
                       size_t userCapacity,      /*!< maximum size to copy     */
                       loff_t* pOffset )         /*!< pointer to the already copied bytes */
{
   DEBUG_MESSAGE( ": Minor-number: %d\n", ((INSTANCE_T*)pFile->private_data)->minor);
   return 0;
}

/*!----------------------------------------------------------------------------
 * @brief
 */
static ssize_t onWrite( struct file *pFile,
                        const char __user* pUserBuffer,
                        size_t len,
                        loff_t* pOffset )
{
   size_t n;
   char  tmp[256];
   INSTANCE_T* pInstance;
   long period;
   int ret;

   memset( tmp, 0, sizeof(tmp) );
   n = min( ARRAY_SIZE( tmp )-1, len );

   pInstance = (INSTANCE_T*)pFile->private_data;

   DEBUG_MESSAGE( ": Minor-number: %d\n", pInstance->minor);
   if( copy_from_user( tmp, pUserBuffer, n ) != 0 )
   {
      ERROR_MESSAGE( "copy_from_user\n" );
      return -EFAULT;
   }

   ret = kstrtol( tmp, 10, &period );
   if( ret < 0 )
      return ret;

   if( period == 0 )
   {
      DEBUG_MESSAGE( "timer%d suspended\n", pInstance->minor  );
      pInstance->period = ~0;
      mod_timer( &pInstance->timer, ~0 );
   }
   else
   {
      DEBUG_MESSAGE( "new period fot timer%d: %u\n", pInstance->minor, (unsigned int)period );
      pInstance->period = period;
      mod_timer( &pInstance->timer, jiffies + msecs_to_jiffies( pInstance->period ) );
   }



   return n;
}

/*!----------------------------------------------------------------------------
 * @brief
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
  .poll           = onPoll
};

/*!----------------------------------------------------------------------------
 * @brief Callback function of the timer
 */
void onMyTimer( struct timer_list* pTimer )
{
    INSTANCE_T* pInstance = from_timer( pInstance, pTimer, timer );
    DEBUG_MESSAGE( ": Minor-number: %d\n", pInstance->minor );
    /*
     * Restart the timer
     */
    mod_timer( pTimer, jiffies + msecs_to_jiffies( pInstance->period ) );
    wake_up_interruptible( &pInstance->readWaitQueue );
}


/*!----------------------------------------------------------------------------
 * @brief Driver constructor
 */
static int __init driverInit( void )
{
   unsigned int minor, currentMinor;
   DEBUG_MESSAGE("\n");

   if( alloc_chrdev_region( &mg.deviceNumber, 0, MAX_INSTANCES, DEVICE_BASE_FILE_NAME ) < 0 )
   {
      ERROR_MESSAGE( "alloc_chrdev_region\n" );
      return -EIO;
   }

   mg.pObject = cdev_alloc();
   if( mg.pObject == NULL )
   {
      ERROR_MESSAGE( "cdev_alloc\n" );
      goto L_DEVICE_NUMBER;
   }

   mg.pObject->owner = THIS_MODULE;
   mg.pObject->ops = &mg_fops;
   if( cdev_add( mg.pObject, mg.deviceNumber, MAX_INSTANCES ) )
   {
      ERROR_MESSAGE( "cdev_add\n" );
      goto L_REMOVE_DEV;
   }
  /*!
   * Register of the driver-instances visible in /sys/class/DEVICE_BASE_FILE_NAME
   */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 0, 0)
   mg.pClass = class_create( DEVICE_BASE_FILE_NAME );
#else
   mg.pClass = class_create( THIS_MODULE, DEVICE_BASE_FILE_NAME );
#endif
   if( IS_ERR( mg.pClass ) )
   {
      ERROR_MESSAGE( "class_create: No udev support\n" );
      goto L_CLASS_REMOVE;
   }

   for( minor = 0; minor < MAX_INSTANCES; minor++ )
   {
      currentMinor = minor;
      if( device_create( mg.pClass,
                         NULL,
                         mg.deviceNumber | minor,
                         NULL,
                         DEVICE_BASE_FILE_NAME "%d",
                         minor )
          == NULL )
      {
         ERROR_MESSAGE( "device_create: " DEVICE_BASE_FILE_NAME "%d\n", minor );
         goto L_INSTANCE_REMOVE;
      }

      mg.instance[minor].minor = minor;
      mg.instance[minor].period = PERIOD_MS * (minor + 1);

      init_waitqueue_head( &mg.instance[minor].readWaitQueue );
      mutex_init( &mg.instance[minor].oMutex );
      timer_setup( &mg.instance[minor].timer, onMyTimer, 0 );
   }

   return 0;

L_INSTANCE_REMOVE:
   for( minor = 0; minor < currentMinor; minor++ )
      device_destroy( mg.pClass, mg.deviceNumber | minor );

L_CLASS_REMOVE:
   class_destroy( mg.pClass );

L_REMOVE_DEV:
   kobject_put( &mg.pObject->kobj );

L_DEVICE_NUMBER:
   unregister_chrdev_region( mg.deviceNumber, 1 );

   return -EIO;
}

/*!----------------------------------------------------------------------------
 * @brief Driver destructor
 */
static void __exit driverExit( void )
{
    int minor;
    DEBUG_MESSAGE("\n");

    for( minor = 0; minor < MAX_INSTANCES; minor++ )
    {
       del_timer_sync( &mg.instance[minor].timer );
       device_destroy( mg.pClass, mg.deviceNumber | minor );
    }
    class_destroy( mg.pClass );
    cdev_del( mg.pObject );
    unregister_chrdev_region( mg.deviceNumber, MAX_INSTANCES );
}

module_init( driverInit );
module_exit( driverExit );
/*================================== EOF ====================================*/
