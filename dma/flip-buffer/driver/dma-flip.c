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
#include <linux/miscdevice.h>
#include <linux/mutex.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/poll.h>
#include <linux/dma-mapping.h>

#include <stdbool.h>

#include <flip_dma_ctl.h>

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
   unsigned int sequence;
   bool         dataReady;
   void*        pDmaBuffers[NUM_BUFFERS];
   dma_addr_t   dmaHandlers[NUM_BUFFERS];
};

struct GLOBAL_T
{
   struct miscdevice        miscdev;
   struct task_struct*      pThread;
   struct mutex             dmaFlipMutex;
   wait_queue_head_t        waitFlipQueue;
   struct DMA_FLIP_BUFFER_T oDmaFlip;
};

static struct GLOBAL_T global =
{
   .oDmaFlip.sequence = 0,
   .oDmaFlip.dataReady = false
};

/*-----------------------------------------------------------------------------
 */
static int onMmap( struct file* pFile, struct vm_area_struct* pVma )
{
   DEBUG_MESSAGE( "\n" );
   const unsigned long len = pVma->vm_end - pVma->vm_start;
   if( len > (NUM_BUFFERS * BUFFER_SIZE) )
   {
      ERROR_MESSAGE( "Data size is too large!\n" );
      return -EINVAL;
   }

   int ret = dma_mmap_coherent( global.miscdev.this_device,
                                pVma,
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
   mutex_lock( &global.dmaFlipMutex );
   bool dataReady = global.oDmaFlip.dataReady;
   mutex_unlock( &global.dmaFlipMutex );
   if( dataReady )
      return POLLIN | POLLRDNORM;
   return 0;
}

/*-----------------------------------------------------------------------------
 */
static long onIoctl( struct file* pFile, unsigned int cmd, unsigned long arg )
{
   DEBUG_MESSAGE( "\n" );
   if( cmd == DMAFLIP_IOCTL_GET_SEQUENCE )
   {
      mutex_lock( &global.dmaFlipMutex );
      if( !global.oDmaFlip.dataReady )
      {
         mutex_unlock( &global.dmaFlipMutex );
         return -EAGAIN;
      }
      typeof(global.oDmaFlip.sequence) sequence = global.oDmaFlip.sequence - 1;
      if( copy_to_user( (typeof(sequence) __user *)arg, &sequence, sizeof(sequence)) != 0 )
      {
         mutex_unlock( &global.dmaFlipMutex );
         ERROR_MESSAGE( "copy_to_user\n" );
         return -EFAULT;
      }
      global.oDmaFlip.dataReady = false;
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
      int sequence = global.oDmaFlip.sequence;
      snprintf( global.oDmaFlip.pDmaBuffers[SEQUENCE_TO_BUFFER_NO(sequence)], BUFFER_SIZE,
                "DMA-Buffer %d full, count = %d", SEQUENCE_TO_BUFFER_NO(sequence), count++ );

      sequence++;
      mutex_lock( &global.dmaFlipMutex );
      global.oDmaFlip.sequence = sequence;
      mutex_unlock( &global.dmaFlipMutex );

      /*
       * Simulating execution-time.
       */
      ssleep( 1 );
      mutex_lock( &global.dmaFlipMutex );
      global.oDmaFlip.dataReady = true;
      mutex_unlock( &global.dmaFlipMutex );
      wake_up_interruptible( &global.waitFlipQueue );
   }
   DEBUG_MESSAGE( " Thread terminated!\n" );
   return 0;
}

/*-----------------------------------------------------------------------------
 */
static int __init onInit( void )
{
   global.miscdev.minor = MISC_DYNAMIC_MINOR;
   global.miscdev.name  = DEVICE_NAME;
   global.miscdev.fops  = &dmaflip_fops;
   if( misc_register( &global.miscdev ) != 0 )
   {
      ERROR_MESSAGE( "misc_register\n" );
      return -ENODEV;
   }

   mutex_init( &global.dmaFlipMutex );
   init_waitqueue_head( &global.waitFlipQueue );

   struct device* pDev = global.miscdev.this_device;
   if( pDev->dma_mask == NULL )
      pDev->dma_mask = &pDev->coherent_dma_mask;
   pDev->coherent_dma_mask = DMA_BIT_MASK(32);

   global.oDmaFlip.pDmaBuffers[0] = dma_alloc_coherent( pDev,
                                                        TOTAL_BUFFER_SIZE,
                                                        &global.oDmaFlip.dmaHandlers[0], GFP_KERNEL );
   if( global.oDmaFlip.pDmaBuffers[0] == NULL )
   {
      ERROR_MESSAGE( "dma_alloc_coherent" );
      misc_deregister( &global.miscdev );
      return -ENOMEM;
   }

   global.oDmaFlip.pDmaBuffers[1] = global.oDmaFlip.pDmaBuffers[0] + BUFFER_SIZE;
   global.oDmaFlip.dmaHandlers[1] = global.oDmaFlip.dmaHandlers[0] + BUFFER_SIZE;

   global.pThread = kthread_run( threadFunction, NULL, "dmaflip_writer" );
   if( IS_ERR( global.pThread ) )
   {
      ERROR_MESSAGE( "kthread_run\n" );
      dma_free_coherent( pDev, TOTAL_BUFFER_SIZE,
                         global.oDmaFlip.pDmaBuffers[0], global.oDmaFlip.dmaHandlers[0]);
      misc_deregister( &global.miscdev );
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
   dma_free_coherent( global.miscdev.this_device, TOTAL_BUFFER_SIZE,
                      global.oDmaFlip.pDmaBuffers[0], global.oDmaFlip.dmaHandlers[0]);
   misc_deregister( &global.miscdev );
   INFO_MESSAGE( "Module successful removed.\n" );
}

module_init(onInit);
module_exit(onExit);

/*================================== EOF ====================================*/
