/*****************************************************************************/
/*                                                                           */
/*!  @brief Simple test driver for DMA accesses and mapping to user-space    */
/*                                                                           */
/*---------------------------------------------------------------------------*/
/*! @file    dma-test.c                                                      */
/*! @author  Ulrich Becker                                                   */
/*! @date    10.04.2025                                                      */
/*****************************************************************************/
#include <linux/version.h>
#include <linux/printk.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/dma-mapping.h>

MODULE_LICENSE( "GPL" );

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


#define DMA_BUFFER_SIZE 4096


struct GLOBAL_T
{
   void*          pDmaVirt;
   dma_addr_t     pDmaPhys;
   struct device* pDev;
};

static struct GLOBAL_T global;

/*!----------------------------------------------------------------------------
 */
static ssize_t onRead( struct file* pFile,
                       char __user* pUserBuffer,
                       size_t len,
                       loff_t* pOffset )
{
   DEBUG_MESSAGE( "minor: %d\n", ((struct miscdevice*)pFile->private_data)->minor );
   if( *pOffset >= DMA_BUFFER_SIZE )
      return 0;

   if( len > DMA_BUFFER_SIZE - *pOffset )
      len = DMA_BUFFER_SIZE - *pOffset;

   if( copy_to_user( pUserBuffer, global.pDmaVirt + *pOffset, len ) != 0 )
   {
      ERROR_MESSAGE( "copy_to_user\n" );
      return -EFAULT;
   }
   *pOffset += len;
   return len;
}

/*!----------------------------------------------------------------------------
 */
static ssize_t onWrite( struct file *pFile,
                        const char __user* pUserBuffer,
                        size_t len,
                        loff_t* pOffset )
{
   DEBUG_MESSAGE( "minor: %d\n", ((struct miscdevice*)pFile->private_data)->minor );
   if( *pOffset >= DMA_BUFFER_SIZE )
   {
      ERROR_MESSAGE( "*pOffset >= DMA_BUFFER_SIZE\n" );
      return -ENOMEM;
   }

   if( len > DMA_BUFFER_SIZE - *pOffset )
      len = DMA_BUFFER_SIZE - *pOffset;

   if( copy_from_user( global.pDmaVirt + *pOffset, pUserBuffer, len ) != 0 )
   {
      ERROR_MESSAGE( "copy_from_user\n" );
      return -EFAULT;
   }

   *pOffset += len;
   return len;
}

/*!----------------------------------------------------------------------------
 */
static int onMmap( struct file* pFile, struct vm_area_struct* pVma )
{
    int ret;

    DEBUG_MESSAGE( "minor: %d\n", ((struct miscdevice*)pFile->private_data)->minor );
    INFO_MESSAGE( "size = %lu\n", pVma->vm_end - pVma->vm_start);

    ret = dma_mmap_coherent( global.pDev, pVma, global.pDmaVirt, global.pDmaPhys, DMA_BUFFER_SIZE );
    if( ret != 0 )
        ERROR_MESSAGE("dma_mmap_coherent failed: %d\n", ret );
    return ret;
}

static const struct file_operations mg_fops =
{
   .owner = THIS_MODULE,
   .read  = onRead,
   .write = onWrite,
   .mmap  = onMmap
};

static struct miscdevice mg_miscdev =
{
   .minor = MISC_DYNAMIC_MINOR,
   .name  = DEVICE_BASE_FILE_NAME,
   .fops  = &mg_fops
};

/*!----------------------------------------------------------------------------
 * @brief Driver constructor
 */
static int __init driverInit( void )
{
   DEBUG_MESSAGE( "\n" );
   int ret;

   ret = misc_register( &mg_miscdev );
   if( ret != 0 )
   {
      ERROR_MESSAGE("misc_register\n");
      return ret;
   }

   global.pDev = mg_miscdev.this_device;

   if( global.pDev->dma_mask == NULL )
       global.pDev->dma_mask = &global.pDev->coherent_dma_mask;
   global.pDev->coherent_dma_mask = DMA_BIT_MASK(32);  // oder 64, je nach Plattform

   global.pDmaVirt = dma_alloc_coherent( global.pDev, DMA_BUFFER_SIZE, &global.pDmaPhys, GFP_KERNEL );
   if( global.pDmaVirt == NULL )
   {
      ERROR_MESSAGE( "dma_alloc_coherent\n" );
      misc_deregister( &mg_miscdev );
      return -ENOMEM;
   }

   INFO_MESSAGE(" loaded, virt=0x%p, phys=%pad\n", global.pDmaVirt, &global.pDmaPhys );
   return 0;
}

/*!----------------------------------------------------------------------------
 * @brief Driver destructor
 */
static void __exit driverExit( void )
{
   DEBUG_MESSAGE( "\n" );

   dma_free_coherent(global.pDev, DMA_BUFFER_SIZE, global.pDmaVirt, global.pDmaPhys);
   misc_deregister( &mg_miscdev );
}

module_init( driverInit );
module_exit( driverExit );
/*================================== EOF ====================================*/
