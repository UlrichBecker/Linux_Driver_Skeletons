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
      perror( ESC_ERROR "open" DEV_FILE ESC_NORMAL );
      return EXIT_FAILURE;
   }

   void* pMappedMems[NUM_BUFFERS];
   pMappedMems[0] = mmap( NULL, TOTAL_BUFFER_SIZE, PROT_READ, MAP_SHARED, fd, 0 );
   if( pMappedMems[0] == MAP_FAILED )
   {
      perror( ESC_ERROR "mmap" ESC_NORMAL );
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
      {
         perror( ESC_ERROR "select" ESC_NORMAL );
         break;
      }
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
         unsigned int sequence;
         if( ioctl( fd, DMAFLIP_IOCTL_GET_SEQUENCE, &sequence) < 0 )
         {
            perror( ESC_ERROR "ioctl" ESC_NORMAL );
            break;
         }
         unsigned int bufferIndex = SEQUENCE_TO_BUFFER_NO( sequence );
         printf( "User reads sequence: %d, buffer[%d]: " ESC_FG_MAGENTA "\"%s\"\n" ESC_NORMAL,
                  sequence, bufferIndex, (char*)pMappedMems[bufferIndex] );
      }
   }
   while( inKey != '\e' );

   munmap( pMappedMems[0], TOTAL_BUFFER_SIZE );
   close( fd );
   resetTerminalInput();
   return EXIT_SUCCESS;
}

/*================================== EOF ====================================*/

