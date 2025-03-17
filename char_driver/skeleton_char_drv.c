/*****************************************************************************/
/*                                                                           */
/*!      @brief Skeleton of a simple Linux character device-driver           */
/*                                                                           */
/*!      For single and multi-instances, depending on MAX_INSTANCES          */
/*                                                                           */
/*---------------------------------------------------------------------------*/
/*! @file    skeleton_char_drv.c                                             */
/*! @author  Ulrich Becker                                                   */
/*! @date    03.02.2017                                                      */
/*****************************************************************************/
#include <linux/version.h>
#include <linux/printk.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#ifdef CONFIG_PROC_FS
   #include <linux/proc_fs.h>
   #include <linux/seq_file.h>
#endif

MODULE_LICENSE( "GPL" );

/*!
 * @brief Device base file-name appearing in
 *        /sys/class/<base-file-name>/<base-file-name>[minor-number]
 *
 * By the default udev-rule, udev will put the device-file-names as
 * follows: \n
 * 1) When MAX_INSTANCES == 1:
 * @code
 * /dev/<base-file-name>
 * @endcode
 * 2) When MAX_INSTANCES == n:
 * @code
 * /dev/<base-file-name>0 /dev/<base-file-name>1 .. /dev/<base-file-name>n
 * @endcode
 */
#define DEVICE_BASE_FILE_NAME KBUILD_MODNAME

/*!
 * @brief Number of driver-instances (minor-numbers).
 */
#define MAX_INSTANCES 10


#ifndef MAX_INSTANCES
  #error Macro MAX_INSTANCES is not defined!
#endif
#if MAX_INSTANCES == 0
  #error Macro MAX_INSTANCES shall be at least 1
#endif

#if defined( CONFIG_PROC_FS ) || defined(__DOXYGEN__)
   /*! @brief Definition of the name in the process file system. */
   #define PROC_FS_NAME "driver/"DEVICE_BASE_FILE_NAME
#endif

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
 * @brief Object-type of private-data for each driver-instance.
 */
typedef struct
{
   int      minor;
   atomic_t openCount;
   /*
    * Further attributes for your application ...
    */
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
#if MAX_INSTANCES > 1
   INSTANCE_T              instance[MAX_INSTANCES];
#else
   INSTANCE_T              instance;
#endif
#ifdef CONFIG_PROC_FS
   struct proc_dir_entry*  poProcFile;
#endif
} MODULE_GLOBAL_T;

static MODULE_GLOBAL_T mg_module;

/* Device file operations begin **********************************************/
/*!----------------------------------------------------------------------------
 * @brief Callback function becomes invoked by the function open() from the
 *        user-space.
 */
static int onOpen( struct inode* pInode, struct file* pInstance )
{
   DEBUG_MESSAGE( ": Minor-number: %d\n", MINOR(pInode->i_rdev) );
   BUG_ON( pInstance->private_data != NULL );
#if MAX_INSTANCES > 1
   BUG_ON( MINOR(pInode->i_rdev) >= MAX_INSTANCES );
   pInstance->private_data = &mg_module.instance[ MINOR(pInode->i_rdev) ];
   atomic_inc( &mg_module.instance[ MINOR(pInode->i_rdev) ].openCount );
   DEBUG_MESSAGE( ":   Open-counter: %d\n", 
                  atomic_read( &mg_module.instance[ MINOR(pInode->i_rdev) ].openCount ));
#else
   atomic_inc( &mg_module.instance.openCount );
   DEBUG_MESSAGE( ":   Open-counter: %d\n", 
                  atomic_read( &mg_module.instance.openCount ));
#endif
   return 0;
}

/*!----------------------------------------------------------------------------
 * @brief Callback function becomes invoked by the function close() from the
 *        user-space.
 */
static int onClose( struct inode *pInode, struct file* pInstance )
{
   DEBUG_MESSAGE( ": Minor-number: %d\n", MINOR(pInode->i_rdev) );
#if MAX_INSTANCES > 1
   BUG_ON( pInstance->private_data == NULL );
   atomic_dec( &mg_module.instance[ MINOR(pInode->i_rdev) ].openCount );
   DEBUG_MESSAGE( "   Open-counter: %d\n", 
                  atomic_read( &mg_module.instance[ MINOR(pInode->i_rdev) ].openCount ));
#else
   atomic_dec( &mg_module.instance.openCount );
   DEBUG_MESSAGE( "   Open-counter: %d\n", 
                  atomic_read( &mg_module.instance.openCount ));
#endif
   return 0;
}

/*!----------------------------------------------------------------------------
 * @brief Callback function becomes invoked by the function read() from the
 *        user-space.
 * @note The kernel invokes onRead as many times till it returns 0 !!!
 */
static ssize_t onRead( struct file* pInstance,   /*!< @see include/linux/fs.h   */
                       char __user* pBuffer,     /*!< buffer to fill with data */
                       size_t len,               /*!< length of the buffer     */
                       loff_t* pOffset )
{
   char tmp[256];
   ssize_t n;

   DEBUG_MESSAGE( ": len = %ld, offset = %lld\n", (long int)len, *pOffset );
   DEBUG_ACCESSMODE( pInstance );

#if MAX_INSTANCES > 1
   BUG_ON( pInstance->private_data == NULL );
   DEBUG_MESSAGE( "   Minor: %d\n", ((INSTANCE_T*)pInstance->private_data)->minor );
   DEBUG_MESSAGE( "   Open-counter: %d\n",
                   atomic_read( &((INSTANCE_T*)pInstance->private_data)->openCount ));

   n = snprintf( tmp,
                 min( len, sizeof(tmp) ), 
                 "Hello world from instance: %d\n",
                ((INSTANCE_T*)pInstance->private_data)->minor );
#else
   DEBUG_MESSAGE( "   Open-counter: %d\n", 
                  atomic_read( &mg_module.instance.openCount ));

   n = snprintf( tmp,
                 min( len, sizeof(tmp) ),
                 "Hello world\n" );
#endif

   n -= (*pOffset);
   if( n <= 0 )
      return n;

   if( copy_to_user( pBuffer, tmp, n ) != 0 )
   {
      ERROR_MESSAGE( "copy_to_user\n" );
      return -EFAULT;
   }

   (*pOffset) += n;
   return n;
   /* Number of bytes successfully read. */
}

/*!----------------------------------------------------------------------------
 * @brief Callback function becomes invoked by the function write() from the
 *        user-space.
 */
static ssize_t onWrite( struct file *pInstance,
                        const char __user* pBuffer,
                        size_t len,
                        loff_t* pOffset )
{
   DEBUG_MESSAGE( ": len = %ld, offset = %lld\n", (long int)len, *pOffset );
   DEBUG_ACCESSMODE( pInstance );

#if MAX_INSTANCES > 1
   BUG_ON( pInstance->private_data == NULL );
   DEBUG_MESSAGE( "   Minor: %d\n", ((INSTANCE_T*)pInstance->private_data)->minor );
   DEBUG_MESSAGE( "   Open-counter: %d\n",
                   atomic_read( &((INSTANCE_T*)pInstance->private_data)->openCount ));
#else
   DEBUG_MESSAGE( "   Open-counter: %d\n", 
                  atomic_read( &mg_module.instance.openCount ));
#endif
   char tmp[256];
   memset( tmp, 0, sizeof(tmp) );

   size_t n = min( ARRAY_SIZE( tmp )-1, len );

   if( copy_from_user( tmp, pBuffer, n ) != 0 )
   {
      ERROR_MESSAGE( "copy_from_user\n" );
      return -EFAULT;
   }
   INFO_MESSAGE( "Received: %s", tmp );
   return n;
}

/*!----------------------------------------------------------------------------
 * @brief Callback function becomes invoked by the function ioctrl() from the
 *        user-space.
 */
static long onIoctrl( struct file* pInstance,
                      unsigned int cmd,
                      unsigned long arg )
{
   DEBUG_MESSAGE( ": cmd = %d arg = %08lX\n", cmd, arg );
   DEBUG_ACCESSMODE( pInstance );
#if MAX_INSTANCES > 1
   BUG_ON( pInstance->private_data == NULL );
   DEBUG_MESSAGE( "   Minor: %d\n", ((INSTANCE_T*)pInstance->private_data)->minor );
   DEBUG_MESSAGE( "   Open-counter: %d\n",
                   atomic_read( &((INSTANCE_T*)pInstance->private_data)->openCount ));
#else
   DEBUG_MESSAGE( "   Open-counter: %d\n",
                   atomic_read( &mg_module.instance.openCount ));
#endif
   return 0;
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
  .unlocked_ioctl = onIoctrl
};
/* Device file operations end ************************************************/

/* Process-file-system begin *************************************************/
#ifdef CONFIG_PROC_FS
/*-----------------------------------------------------------------------------
 */
static int procOnOpen( struct seq_file* pSeqFile, void* pValue )
{
   DEBUG_MESSAGE( "\n" );
   seq_printf( pSeqFile, "Hello world\n" );
   return 0;
}

/*-----------------------------------------------------------------------------
 */
static int _procOnOpen( struct inode* pInode, struct file *pFile )
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0)
   return single_open( pFile, procOnOpen, NULL );
#else
   return single_open( pFile, procOnOpen, PDE_DATA( pInode ) );
#endif
}

/*-----------------------------------------------------------------------------
 */
static ssize_t procOnWrite( struct file* seq, const char* pData,
                            size_t len, loff_t* pPos )
{
   DEBUG_MESSAGE( "\n" );
   return len;
}

/*-----------------------------------------------------------------------------
 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 0, 0)
static const struct proc_ops mg_procFileOps =
{
  .proc_open    = _procOnOpen,
  .proc_read    = seq_read,
  .proc_write   = procOnWrite,
  .proc_lseek   = seq_lseek,
  .proc_release = single_release
};
#else
static const struct file_operations mg_procFileOps =
{
  .owner   = THIS_MODULE,
  .open    = _procOnOpen,
  .read    = seq_read,
  .write   = procOnWrite,
  .llseek  = seq_lseek,
  .release = single_release
};
#endif
#endif /* ifdef CONFIG_PROC_FS */
/* Process-file-system end ***************************************************/

/* Power management functions begin ******************************************/
#ifdef CONFIG_PM__
/*-----------------------------------------------------------------------------
 */
static int onPmSuspend( struct device* pDev, pm_message_t state )
{
   DEBUG_MESSAGE( "( %p )\n", pDev );
#ifdef CONFIG_DEBUG_SKELETON
   #define CASE_ITEM( s ) case s: DEBUG_MESSAGE( ": " #s "\n" ); break;
   switch( state.event )
   {
      CASE_ITEM( PM_EVENT_ON )
      CASE_ITEM( PM_EVENT_FREEZE )
      CASE_ITEM( PM_EVENT_SUSPEND )
      CASE_ITEM( PM_EVENT_HIBERNATE )
      default:
      {
         DEBUG_MESSAGE( "pm_event: 0x%X\n", state.event );
         break;
      }
  }
  #undef CASE_ITEM
#endif
  return 0;
}

/*-----------------------------------------------------------------------------
 */
static int onPmResume( struct device* pDev )
{
  DEBUG_MESSAGE( "(%p)\n", pDev );
  return 0;
}

#endif /* ifdef CONFIG_PM */
/* Power management functions end ********************************************/

/*!----------------------------------------------------------------------------
 * @brief Driver constructor
 */
static int __init driverInit( void )
{
#if MAX_INSTANCES > 1
   int minor, currentMinor;
#endif
   DEBUG_MESSAGE("\n");

   if( alloc_chrdev_region( &mg_module.deviceNumber, 0, MAX_INSTANCES, DEVICE_BASE_FILE_NAME ) < 0 )
   {
      ERROR_MESSAGE( "alloc_chrdev_region\n" );
      return -EIO;
   }

   mg_module.pObject = cdev_alloc();
   if( mg_module.pObject == NULL )
   {
      ERROR_MESSAGE( "cdev_alloc\n" );
      goto L_DEVICE_NUMBER;
   }

   mg_module.pObject->owner = THIS_MODULE;
   mg_module.pObject->ops = &mg_fops;
   if( cdev_add( mg_module.pObject, mg_module.deviceNumber, MAX_INSTANCES ) )
   {
      ERROR_MESSAGE( "cdev_add\n" );
      goto L_REMOVE_DEV;
   }

  /*!
   * Register of the driver-instances visible in /sys/class/DEVICE_BASE_FILE_NAME
   */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 0, 0)
   mg_module.pClass = class_create( DEVICE_BASE_FILE_NAME );
#else
   mg_module.pClass = class_create( THIS_MODULE, DEVICE_BASE_FILE_NAME );
#endif
   if( IS_ERR(mg_module.pClass) )
   {
      ERROR_MESSAGE( "class_create: No udev support\n" );
      goto L_CLASS_REMOVE;
   }

#if MAX_INSTANCES > 1
   for( minor = 0; minor < MAX_INSTANCES; minor++ )
   {
      currentMinor = minor;
      if( device_create( mg_module.pClass,
                         NULL,
                         mg_module.deviceNumber | minor,
                         NULL,
                         DEVICE_BASE_FILE_NAME "%d",
                         minor )
          == NULL )
      {
         ERROR_MESSAGE( "device_create: " DEVICE_BASE_FILE_NAME "%d\n", minor );
         goto L_INSTANCE_REMOVE;
      }
      mg_module.instance[minor].minor = minor;
      atomic_set( &mg_module.instance[minor].openCount, 0 );
      DEBUG_MESSAGE( ": Instance " DEVICE_BASE_FILE_NAME "%d created\n", minor );
   }
   currentMinor = MAX_INSTANCES;
#else
   if( device_create( mg_module.pClass,
                      NULL,
                      mg_module.deviceNumber,
                      NULL,
                      DEVICE_BASE_FILE_NAME )
      == NULL )
   {
      ERROR_MESSAGE( "device_create: " DEVICE_BASE_FILE_NAME "\n" );
      goto L_INSTANCE_REMOVE;
   }
   mg_module.instance.minor = 0;
   atomic_set( &mg_module.instance.openCount, 0 );
   DEBUG_MESSAGE( ": Instance " DEVICE_BASE_FILE_NAME " created\n" );
#endif

#ifdef CONFIG_PM__
  mg_module.pClass->suspend = onPmSuspend;
  mg_module.pClass->resume =  onPmResume;
#endif

#ifdef CONFIG_PROC_FS
 #if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0)
   mg_module.poProcFile = proc_create_data( PROC_FS_NAME,
                                            S_IRUGO | S_IWUGO, NULL,
                                            &mg_procFileOps,
                                            NULL ); //proc_data );
 #else
   mg_module.poProcFile = proc_create( PROC_FS_NAME,
                                       S_IRUGO | S_IWUGO,
                                       NULL,
                                       &mg_procFileOps );
 #endif
   if( mg_module.poProcFile == NULL )
   {
      ERROR_MESSAGE( "Unable to create proc entry: /proc/"PROC_FS_NAME" !\n" );
      goto L_INSTANCE_REMOVE;
   }
#endif

   return 0;


L_INSTANCE_REMOVE:
#if MAX_INSTANCES > 1
   for( minor = 0; minor < currentMinor; minor++ )
      device_destroy( mg_module.pClass, mg_module.deviceNumber | minor );
#else
   device_destroy( mg_module.pClass, mg_module.deviceNumber );
#endif

L_CLASS_REMOVE:
   class_destroy( mg_module.pClass );

L_REMOVE_DEV:
   kobject_put( &mg_module.pObject->kobj );

L_DEVICE_NUMBER:
   unregister_chrdev_region( mg_module.deviceNumber, 1 );

   return -EIO;
}

/*!----------------------------------------------------------------------------
 * @brief Driver destructor
 */
static void __exit driverExit( void )
{
#if MAX_INSTANCES > 1
  int minor;
#endif
  DEBUG_MESSAGE("\n");
#ifdef CONFIG_PROC_FS
  remove_proc_entry( PROC_FS_NAME, NULL );
#endif

#if MAX_INSTANCES > 1
  for( minor = 0; minor < MAX_INSTANCES; minor++ )
     device_destroy( mg_module.pClass, mg_module.deviceNumber | minor );
#else
  device_destroy( mg_module.pClass, mg_module.deviceNumber );
#endif
  class_destroy( mg_module.pClass );
  cdev_del( mg_module.pObject );
  unregister_chrdev_region( mg_module.deviceNumber, MAX_INSTANCES );
  return;
}

/*-----------------------------------------------------------------------------
 */
module_init( driverInit );
module_exit( driverExit );
/*================================== EOF ====================================*/
