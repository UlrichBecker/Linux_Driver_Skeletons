/*****************************************************************************/
/*                                                                           */
/*!      @brief Simple test driver for DMA accesses                          */
/*                                                                           */
/*---------------------------------------------------------------------------*/
/*! @file    dma-test.c                                                      */
/*! @author  Ulrich Becker                                                   */
/*! @date    03.04.2025                                                      */
/*****************************************************************************/
#include <linux/version.h>
#include <linux/printk.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/io.h>

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

/*!----------------------------------------------------------------------------
 * @brief Driver constructor
 */
static int __init driverInit( void )
{
   DEBUG_MESSAGE( "\n" );
   return 0;
}

/*!----------------------------------------------------------------------------
 * @brief Driver destructor
 */
static void __exit driverExit( void )
{
   DEBUG_MESSAGE( "\n" );
}

module_init( driverInit );
module_exit( driverExit );
/*================================== EOF ====================================*/
