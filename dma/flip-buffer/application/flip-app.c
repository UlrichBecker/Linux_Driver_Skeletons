/*****************************************************************************/
/*                                                                           */
/*! @brief Application part in user-space for testing the dma-flip driver    */
/*!        dmaflip.ko                                                        +/
/*                                                                           */
/*---------------------------------------------------------------------------*/
/*! @file   flip-app.c                                                       */
/*! @author Ulrich Becker                                                    */
/*! @date   15.04.2025                                                       */
/*****************************************************************************/
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <terminalHelper.h>
#include <flip_dma_ctl.h>

#define DEV_FILE "/dev/" DEVICE_NAME

int main( void )
{
   printf( "DMA memory map flip test\n"
           "Press Esc for end.\n"
           "opening devicefile: " DEV_FILE "\n" );
   const int fd = open( DEV_FILE, O_RDONLY );
   if( fd < 0 )
   {
      perror( "open" DEV_FILE );
      return EXIT_FAILURE;
   }

   void* pMappedMems[NUM_BUFFERS];
   pMappedMems[0] = mmap( NULL, BUFFER_SIZE * NUM_BUFFERS, PROT_READ, MAP_SHARED, fd, 0 );
   if( pMappedMems[0] == MAP_FAILED )
   {
      perror("mmap");
      return EXIT_FAILURE;
   }
   pMappedMems[1] = pMappedMems[0] + BUFFER_SIZE;

   if( prepareTerminalInput() != 0 )
      return EXIT_FAILURE;

   int fdMax = STDIN_FILENO;
   if( fdMax < fd )
      fdMax = fd;
   fdMax++;

   fd_set rfds;
   int inKey = 0;
   do
   {
      FD_ZERO( &rfds );
      FD_SET( STDIN_FILENO, &rfds );
      FD_SET( fd, &rfds );
      int state = select( fdMax, &rfds, NULL, NULL, NULL );
      if( state < 0 )
         break;
      if( state == 0 )
         continue;
      if( FD_ISSET( STDIN_FILENO, &rfds ) )
      {
         if( read( STDIN_FILENO, &inKey, sizeof( inKey ) ) > 0 )
         {
            inKey &= 0xFF;
            if( inKey == '\e' )
            {
               printf( "End...\n" );
            }
         }
      }
      if( FD_ISSET( fd, &rfds ) )
      {
         int ready;
         unsigned int readIndex;
         if( ioctl( fd, DMAFLIP_IOCTL_GET_READY, &readIndex) < 0 )
         {
            perror("ioctl");
            break;
         }
         printf( "User reads buffer[%d]: %s\n", readIndex, (char*)pMappedMems[readIndex] );
      }
   }
   while( inKey != '\e' );

   munmap( pMappedMems[0], BUFFER_SIZE * NUM_BUFFERS );
   close( fd );
   resetTerminalInput();
   return EXIT_SUCCESS;
}

/*================================== EOF ====================================*/

