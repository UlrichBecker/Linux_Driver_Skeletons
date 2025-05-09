/*****************************************************************************/
/*                                                                           */
/*! @brief Demonstration of the callback function poll cooperating between   */
/*!        kernel-space and user application select()                        */
/*                                                                           */
/*---------------------------------------------------------------------------*/
/*! @file    poll-drv.c                                                      */
/*! @author  Ulrich Becker                                                   */
/*! @date    05.10.2017                                                      */
/*****************************************************************************/
#include <linux/version.h>
#include <linux/printk.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/poll.h>
#include <linux/uaccess.h>
#include <linux/wait.h>
#include <linux/sched.h>


MODULE_LICENSE( "GPL" );

/*!
 * @brief Device base file-name appearing in
 *        /sys/class/<base-file-name>/<base-file-name>[minor-number]
 *
 */
#define DEVICE_BASE_FILE_NAME KBUILD_MODNAME

/*!
 * @brief Number of driver-instances (minor-numbers).
 */
#define MAX_INSTANCES 2


#ifndef MAX_INSTANCES
  #error Macro MAX_INSTANCES is not defined!
#endif
#if MAX_INSTANCES == 0
  #error Macro MAX_INSTANCES shall be at least 1
#endif


/* Begin of message helper macros for "dmesg" *********************************/
/* NOTE for newer systems with "systend"
 * "dmesg -w" corresponds the old “tail -f /var/log/messages”
 */
#define ERROR_MESSAGE( constStr, n... ) \
   pr_err( DEVICE_BASE_FILE_NAME "-error %d: %s" constStr, __LINE__, __func__, ## n )

#define DEBUG_MESSAGE( constStr, n... ) \
   pr_debug( DEVICE_BASE_FILE_NAME "-dbg %d: %s" constStr, __LINE__, __func__, ## n )

#if defined( CONFIG_DEBUG_SKELETON ) || defined(__DOXYGEN__)
   #define DEBUG_ACCESSMODE( pFile ) \
      DEBUG_MESSAGE( ": access: %s\n", \
                     (pFile->f_flags & O_NONBLOCK)? "non blocking":"blocking" )
#else
   #define DEBUG_ACCESSMODE( pFile )
#endif

#define INFO_MESSAGE( constStr, n... ) \
   pr_info( DEVICE_BASE_FILE_NAME ": " constStr, ## n )

/* End of message helper macros for "dmesg" ++++++++***************************/

/* End of message helper macros for "dmesg" ++++++++***************************/

/*!
 * @brief Object-type of private-data for each driver-instance.
 */
typedef struct
{
   int               minor;
   atomic_t          openCount;
   struct mutex      oMutex;
   wait_queue_head_t readWaitQueue;
   wait_queue_head_t writeWaitQueue;
   int               index;
   char              buffer[16];
} INSTANCE_T;

/*!
 * @brief Structure of global variables.
 *
 * Of course it's not necessary to put the following variables in a structure
 * but that is a certain kind of commenting the code its self and makes the
 * code better readable.
 */
typedef struct
{
   dev_t                   deviceNumber;
   struct cdev*            pObject;
   struct class*           pClass;
   INSTANCE_T              instance[MAX_INSTANCES];
} MODULE_GLOBAL_T;

static MODULE_GLOBAL_T mg;

/* Device file operations begin **********************************************/
/*!----------------------------------------------------------------------------
 * @brief Callback function becomes invoked by the function open() from the
 *        user-space.
 */
static int onOpen( struct inode* pInode, struct file* pFile )
{
   int instanceIndex = MINOR(pInode->i_rdev);
   INSTANCE_T* pInstance = &mg.instance[ instanceIndex ];

   DEBUG_MESSAGE( ": Minor-number: %d\n", instanceIndex );
   BUG_ON( pFile->private_data != NULL );
   BUG_ON( instanceIndex >= MAX_INSTANCES );

   pFile->private_data = pInstance;

   if( atomic_inc_and_test( &pInstance->openCount ) == 1 )
   {
      //memset( &pInstance->buffer, 0, sizeof(pInstance->buffer) );
   }
   DEBUG_MESSAGE( ": Open-counter: %d\n", atomic_read( &pInstance->openCount ));
   return 0;
}

/*!----------------------------------------------------------------------------
 * @brief Callback function becomes invoked by the function close() from the
 *        user-space.
 */
static int onClose( struct inode *pInode, struct file* pFile )
{
   DEBUG_MESSAGE( ": Minor-number: %d\n", MINOR(pInode->i_rdev) );
   BUG_ON( pFile->private_data == NULL );
   atomic_dec( &mg.instance[ MINOR(pInode->i_rdev) ].openCount );
   DEBUG_MESSAGE( "   Open-counter: %d\n", 
                  atomic_read( &mg.instance[ MINOR(pInode->i_rdev) ].openCount ));
   return 0;
}

/*!----------------------------------------------------------------------------
 * @brief Callback function becomes invoked by the function read() from the
 *        user-space.
 * @note The kernel invokes onRead as many times till it returns 0 !!!
 */
static ssize_t onRead( struct file* pFile,       /*!< @see include/linux/fs.h   */
                       char __user* pUserBuffer, /*!< buffer to fill with data */
                       size_t userCapacity,      /*!< maximum size to copy     */
                       loff_t* pOffset )         /*!< pointer to the already copied bytes */
{
   ssize_t remaining;
   size_t  copyLen;
   INSTANCE_T* pInstance = pFile->private_data;

   DEBUG_MESSAGE( ": userCapacity = %ld, offset = %lld\n", (long int)userCapacity, *pOffset );
   DEBUG_ACCESSMODE( pFile );

   BUG_ON( pInstance == NULL );
   DEBUG_MESSAGE( "   Minor: %d\n", pInstance->minor );
   DEBUG_MESSAGE( "   Open-counter: %d\n",
                   atomic_read( &pInstance->openCount ));

   if( (*pOffset > 0) && (pInstance->index == *pOffset))
   {
      pInstance->index = 0;
      *pOffset = 0;
      wake_up_interruptible( &pInstance->writeWaitQueue );
      return 0;
   }

   if( pInstance->index == 0 ) /* No data to read present? */
   {
      if( pFile->f_flags & O_NONBLOCK )
         return -EAGAIN; /* non blocking */
      /*!
       * @note At the first look the following code-line seems very
       *       confusing (specially the second parameter "(pInstance->index > 0)") \n
       *       when we look at the enclosing if-condition of
       *       "pInstance->index == 0" above. \n
       *       But "wait_event_interruptible" isn't a function rather a macro
       *       defined in "include/linux/wait.h" \n
       *       and "pInstance" is a pointer, that means his content can be
       *       modified by a concurrent task e.g.: by a write-call.
       */
      if( wait_event_interruptible( pInstance->readWaitQueue, (pInstance->index > 0) ) != 0)
         return -ERESTARTSYS;  /* Loop */
   }

   copyLen = min( userCapacity, sizeof(pInstance->buffer) );
   if( copyLen > pInstance->index )
      copyLen = pInstance->index;

   remaining = copy_to_user( pUserBuffer, &pInstance->buffer[*pOffset], copyLen - *pOffset );
   if( remaining < 0 )
      return -EFAULT; // Data not compleaty copied.

   copyLen  -= remaining;
   *pOffset += copyLen;

   return copyLen; /* Number of bytes successfully read. */
}

/*!----------------------------------------------------------------------------
 * @brief Callback function becomes invoked by the function write() from the
 *        user-space.
 */
static ssize_t onWrite( struct file *pFile,
                        const char __user* pUserBuffer,
                        size_t len,
                        loff_t* pOffset )
{
   INSTANCE_T* pInstance = pFile->private_data;

   BUG_ON( pInstance == NULL );
   DEBUG_MESSAGE( ": len = %ld, offset = %lld\n", (long int)len, *pOffset );
   DEBUG_ACCESSMODE( pFile );
   DEBUG_MESSAGE( "   Minor: %d\n", pInstance->minor );
   DEBUG_MESSAGE( "   Open-counter: %d\n",
                   atomic_read( &pInstance->openCount ));

   if( pInstance->index > 0 ) /* Buffer not completely read yet? */
   {
      if( pFile->f_flags & O_NONBLOCK )
         return -EAGAIN; /* non blocking */
      /*!
       * @note At the first look the following code-line seems very
       *       confusing (specially the second parameter "(pInstance->index == 0)") \n
       *       when we look at the enclosing if-condition of
       *       "pInstance->index > 0" above. \n
       *       But "wait_event_interruptible" isn't a function rather a macro
       *       defined in "include/linux/wait.h" \n
       *       and "pInstance" is a pointer, that means his content can be
       *       modified by a concurrent task e.g.: by a read-call.
       */
      if( wait_event_interruptible( pInstance->writeWaitQueue, (pInstance->index == 0) ) != 0)
         return -ERESTARTSYS;  /* Loop */
   }

   if( len > sizeof(pInstance->buffer) )
      len = sizeof(pInstance->buffer);

   if( copy_from_user( pInstance->buffer, pUserBuffer, len ) != 0 )
      return -EFAULT;

   pInstance->index = len;

   wake_up_interruptible( &pInstance->readWaitQueue );

   return len;
}

/*!----------------------------------------------------------------------------
 * @brief Callback function becomes invoked by the function select() from the
 *        user-space.
 */
static unsigned int onPoll( struct file* pFile, poll_table* pPollTable )
{
   INSTANCE_T* pInstance = pFile->private_data;
   unsigned int ret = 0;

   BUG_ON( pInstance == NULL );

   DEBUG_MESSAGE( ": Minor-number: %d\n", ((INSTANCE_T*)pFile->private_data)->minor );

   mutex_lock( &pInstance->oMutex );

   poll_wait( pFile, &pInstance->readWaitQueue, pPollTable );
   poll_wait( pFile, &pInstance->writeWaitQueue, pPollTable );

   if( pInstance->index > 0 )
      ret |= (POLLIN | POLLRDNORM); /* ready to read */

   if( pInstance->index == 0 )
      ret |= (POLLOUT | POLLWRNORM); /* ready to write */

   mutex_unlock( &pInstance->oMutex );

   return ret;
}

/*-----------------------------------------------------------------------------
 */
static struct file_operations mg_fops =
{
  .owner          = THIS_MODULE,
  .open           = onOpen,
  .release        = onClose,
  .read           = onRead,
  .write          = onWrite,
  .poll           = onPoll
};
/* Device file operations end ************************************************/

/*!----------------------------------------------------------------------------
 * @brief Driver constructor
 */
static int __init driverInit( void )
{
   int minor, currentMinor;

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
   if( IS_ERR(mg.pClass) )
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
      atomic_set( &mg.instance[minor].openCount, 0 );
      memset( &mg.instance[minor].buffer, 0, sizeof(mg.instance[minor].buffer) );
      mg.instance[minor].index = 0;

      init_waitqueue_head( &mg.instance[minor].readWaitQueue );
      init_waitqueue_head( &mg.instance[minor].writeWaitQueue );
      mutex_init( &mg.instance[minor].oMutex );

      DEBUG_MESSAGE( ": Instance " DEVICE_BASE_FILE_NAME "%d created\n", minor );
   }
   currentMinor = MAX_INSTANCES;

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
     device_destroy( mg.pClass, mg.deviceNumber | minor );

  class_destroy( mg.pClass );
  cdev_del( mg.pObject );
  unregister_chrdev_region( mg.deviceNumber, MAX_INSTANCES );
  return;
}

/*-----------------------------------------------------------------------------
 */
module_init( driverInit );
module_exit( driverExit );
/*================================== EOF ====================================*/
