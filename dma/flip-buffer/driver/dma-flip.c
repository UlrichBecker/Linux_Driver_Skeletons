/*****************************************************************************/
/*                                                                           */
/*!  @brief Simple test driver for DMA accesses and mapping to user-space    */
/*          with using of flip-buffer                                        */
/*                                                                           */
/*---------------------------------------------------------------------------*/
/*! @file    dma-flip.c                                                      */
/*! @author  Ulrich Becker                                                   */
/*! @date    14.04.2025                                                      */
/*****************************************************************************/
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/mutex.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/poll.h>
#include <linux/dma-mapping.h>

#include <flip_dma_ctl.h>

#define CLASS_NAME  DEVICE_NAME "class"


MODULE_LICENSE("GPL");

#define CONFIG_DEBUG_SKELETON
#define DEVICE_BASE_FILE_NAME KBUILD_MODNAME

/* Begin of message helper macros for "dmesg" *********************************/
/* NOTE for newer systems with "systend"
 * "dmesg -w" corresponds the old “tail -f /var/log/messages”
 */
#define ERROR_MESSAGE( constStr, n... ) \
   printk( KERN_ERR DEVICE_BASE_FILE_NAME "-error %d: %s " constStr, __LINE__, __func__, ## n )

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


struct DMA_FLIP_BUFFER_T
{
   unsigned int readyIndex;
   unsigned int dataReady;
   void*        pDmaBuffers[NUM_BUFFERS];
   dma_addr_t   dmaHandlers[NUM_BUFFERS];
};

struct GLOBAL_T
{
   struct device*           pDmaDevice;
   struct class*            pDmaflipClass;
   struct cdev              pdmaflipCdev;
   int                      major;
   struct task_struct*      pThread;
   struct mutex             dmaFlipMutex;
   wait_queue_head_t        waitFlipQueue;
   struct DMA_FLIP_BUFFER_T oDmaFlip;
};

static struct GLOBAL_T global =
{
   .oDmaFlip.readyIndex = 0,
   .oDmaFlip.dataReady = 0
};

/*-----------------------------------------------------------------------------
 */
static int onMmap( struct file* pFile, struct vm_area_struct* pVma)
{
   DEBUG_MESSAGE( "\n" );
   const unsigned long len = pVma->vm_end - pVma->vm_start;
   if( len > (NUM_BUFFERS * BUFFER_SIZE) )
   {
      ERROR_MESSAGE( "Data size is too large!\n" );
      return -EINVAL;
   }

   int ret = dma_mmap_coherent( global.pDmaDevice, pVma,
                                global.oDmaFlip.pDmaBuffers[0],
                                global.oDmaFlip.dmaHandlers[0],
                                BUFFER_SIZE * NUM_BUFFERS );
   if( ret != 0 )
   {
      ERROR_MESSAGE( "dma_mmap_coherent failed: %d\n", ret );
   }
   return ret;
}

/*-----------------------------------------------------------------------------
 */
static unsigned int onPoll( struct file* pFile, poll_table* pPollTable )
{
   DEBUG_MESSAGE( "\n" );
   poll_wait( pFile, &global.waitFlipQueue, pPollTable );
   if( global.oDmaFlip.dataReady != 0 )
      return POLLIN | POLLRDNORM;
   return 0;
}

/*-----------------------------------------------------------------------------
 */
static long onIoctl( struct file* pFile, unsigned int cmd, unsigned long arg )
{
   DEBUG_MESSAGE( "\n" );
   if( cmd == DMAFLIP_IOCTL_GET_READY )
   {
      mutex_lock( &global.dmaFlipMutex );
      if( global.oDmaFlip.dataReady == 0 )
      {
         mutex_unlock( &global.dmaFlipMutex );
         return -EAGAIN;
      }
      unsigned int readIndex = (global.oDmaFlip.readyIndex + 1) % NUM_BUFFERS;
      if( copy_to_user( (unsigned int __user *)arg, &readIndex, sizeof(readIndex)) != 0 )
      {
         mutex_unlock( &global.dmaFlipMutex );
         ERROR_MESSAGE( "copy_to_user\n" );
         return -EFAULT;
      }
      global.oDmaFlip.dataReady = 0;
      mutex_unlock( &global.dmaFlipMutex );
      return 0;
   }

   return -ENOTTY;
}

/*-----------------------------------------------------------------------------
 */
static int onOpen(struct inode *inode, struct file *file)
{
   DEBUG_MESSAGE( "\n" );
   return 0;
}

/*-----------------------------------------------------------------------------
 */
static int onClose(struct inode *inode, struct file *file)
{
   DEBUG_MESSAGE( "\n" );
   return 0;
}

static const struct file_operations dmaflip_fops =
{
   .owner          = THIS_MODULE,
   .mmap           = onMmap,
   .unlocked_ioctl = onIoctl,
   .open           = onOpen,
   .release        = onClose,
   .poll           = onPoll
};

/*-----------------------------------------------------------------------------
 */
static int threadFunction( void* pData )
{
   int count = 0;
   DEBUG_MESSAGE( " Thread started!\n" );
   while( !kthread_should_stop() )
   {  /*
       * write into the inactive buffer.
       */
      int readyIndex = global.oDmaFlip.readyIndex;

      readyIndex = (readyIndex + 1) % NUM_BUFFERS;
      snprintf( global.oDmaFlip.pDmaBuffers[readyIndex], BUFFER_SIZE,
                "DMA-Buffer %d full, count = %d", readyIndex, count++ );

      mutex_lock( &global.dmaFlipMutex );
      global.oDmaFlip.readyIndex = readyIndex;
      mutex_unlock( &global.dmaFlipMutex );

      /*
       * Simulating execution-time.
       */
      ssleep( 1 );
      global.oDmaFlip.dataReady = 1;
      wake_up_interruptible( &global.waitFlipQueue );
   }
   DEBUG_MESSAGE( " Thread terminated!\n" );
   return 0;
}

/*-----------------------------------------------------------------------------
 */
static int __init onInit( void )
{
   dev_t dev;
   int ret;

   mutex_init( &global.dmaFlipMutex );
   init_waitqueue_head( &global.waitFlipQueue );

   ret = alloc_chrdev_region( &dev, 0, 1, DEVICE_NAME );
   if( ret < 0 )
      return ret;
   global.major = MAJOR(dev);

   cdev_init( &global.pdmaflipCdev, &dmaflip_fops );
   cdev_add( &global.pdmaflipCdev, dev, 1 );

   global.pDmaflipClass = class_create( THIS_MODULE, CLASS_NAME );
   global.pDmaDevice = device_create( global.pDmaflipClass, NULL, dev, NULL, DEVICE_NAME );

   if( global.pDmaDevice->dma_mask == NULL )
      global.pDmaDevice->dma_mask = &global.pDmaDevice->coherent_dma_mask;
   global.pDmaDevice->coherent_dma_mask = DMA_BIT_MASK(32);

   global.oDmaFlip.pDmaBuffers[0] = dma_alloc_coherent( global.pDmaDevice,
                                                        BUFFER_SIZE * NUM_BUFFERS,
                                                        &global.oDmaFlip.dmaHandlers[0], GFP_KERNEL );
   if( global.oDmaFlip.pDmaBuffers[0] == NULL )
   {
      ERROR_MESSAGE( "dma_alloc_coherent" );
      return -ENOMEM;
   }
   global.oDmaFlip.pDmaBuffers[1] = global.oDmaFlip.pDmaBuffers[0] + BUFFER_SIZE;
   global.oDmaFlip.dmaHandlers[1] = global.oDmaFlip.dmaHandlers[0] + BUFFER_SIZE;

   global.pThread = kthread_run( threadFunction, NULL, "dmaflip_writer" );
   if( IS_ERR( global.pThread ) )
   {
      ERROR_MESSAGE( "kthread_run\n" );
      return PTR_ERR( global.pThread );
   }
   INFO_MESSAGE( "Module successful loaded\n" );
   return 0;
}

/*-----------------------------------------------------------------------------
 */
static void __exit onExit(void)
{
   kthread_stop( global.pThread );

   dma_free_coherent( global.pDmaDevice, BUFFER_SIZE * NUM_BUFFERS,
                      global.oDmaFlip.pDmaBuffers[0], global.oDmaFlip.dmaHandlers[0]);

   device_destroy( global.pDmaflipClass, MKDEV(global.major, 0) );
   class_destroy( global.pDmaflipClass );
   cdev_del( &global.pdmaflipCdev );
   unregister_chrdev_region( MKDEV(global.major, 0), 1 );

   INFO_MESSAGE( "Module successful removed.\n" );
}

module_init(onInit);
module_exit(onExit);

/*================================== EOF ====================================*/
