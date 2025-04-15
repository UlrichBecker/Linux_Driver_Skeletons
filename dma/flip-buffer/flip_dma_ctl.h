/*****************************************************************************/
/*                                                                           */
/*!  @brief Common header file for ioctl-commands of the                     */
/*!         test driver for DMA accesses and mapping to user-space           */
/*          with using of flip-buffer                                        */
/*                                                                           */
/*---------------------------------------------------------------------------*/
/*! @file    flip_dma_ctl.h                                                  */
/*! @author  Ulrich Becker                                                   */
/*! @date    14.04.2025                                                      */
/*****************************************************************************/
#ifndef _FLIP_DMA_CTL_H
#define _FLIP_DMA_CTL_H

#ifndef __KERNEL__
 #include <sys/ioctl.h>
 #include <sys/mman.h>
 #include <fcntl.h>
 #include <unistd.h>
#endif

#define DEVICE_NAME "dmaflip"
#define NUM_BUFFERS 2
#define BUFFER_SIZE 4096

#define DMAFLIP_IOCTL_GET_READY _IOR( 'D', 1, int )

#endif /* ifndef _FLIP_DMA_CTL_H */
/*================================== EOF ====================================*/
