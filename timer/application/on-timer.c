/*****************************************************************************/
/*                                                                           */
/*! @brief Application part in user-space to demonstrate how cooperates the  */
/*         function select() with a kernel-module /dev/timerX.               */
/*                                                                           */
/*---------------------------------------------------------------------------*/
/*! @file   on-timer.c                                                       */
/*! @author Ulrich Becker                                                    */
/*! @date   26.03.2025                                                       */
/*****************************************************************************/
/*! @note It's possible to run this program as non-root user if you
 *        creates a new udev-rule:
 *! @code
 * KERNEL=="timer[0-9]", MODE="0666"
 *! @endcode
 * Then restart udev:
 *! @code
 * udevadm control --reload
 *! @endcode
 */
#include <stdio.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <findInstances.h>
#include <terminalHelper.h>

#ifndef ARRAY_SIZE
 #define ARRAY_SIZE( a ) (sizeof( a ) / sizeof( a[0] ))
#endif


#define BASE_NAME "timer"

typedef struct
{
   char  fileName[16];
   int   fd;
   unsigned int recCount;
} POLL_OBJ_T;

char g_textBuffer[64];

int main( void )
{
   ssize_t readBytes;
   printf( _ESC_XY( "1", "1" ) ESC_CLR_SCR  "Test of Linux-kernel-driver \"" BASE_NAME "\"\n"
   "Open a further console and send a message to /dev/" BASE_NAME "0\n"
   "E.g.: \"echo 1000 > /dev/" BASE_NAME "0\" sets a period of 1000 ms\n"
   "      \"echo 0 > /dev/" BASE_NAME "0\" suspends this timer- instance.\n" );

   const int numOfInstances = getNumberOfFoundDriverInstances( BASE_NAME );
   if( numOfInstances < 0 )
   {
      fprintf( stderr, "ERROR: Directory not found!\n" );
      return EXIT_FAILURE;
   }
   printf( "Found driver instances: %d\n", numOfInstances );
   if( numOfInstances == 0 )
   {
      printf( "No driver-instance of " BASE_NAME " found.\n" );
      return EXIT_SUCCESS;
   }

   POLL_OBJ_T* pUsers = malloc( numOfInstances * sizeof(POLL_OBJ_T) );
   if( pUsers == NULL )
   {
      fprintf( stderr, "ERROR: Unable to allocate memory for %d instances!\n", numOfInstances );
      return EXIT_FAILURE;
   }

   if( prepareTerminalInput() != 0 )
   {
      fprintf( stderr, "ERROR Unable to prepare termilal input!\n" );
      return EXIT_FAILURE;
   }

   int fdMax = STDIN_FILENO;
   for( int i = 0; i < numOfInstances; i++ )
   {
      snprintf( pUsers[i].fileName, ARRAY_SIZE( pUsers[0].fileName ), "/dev/" BASE_NAME "%d", i );
      printf( "Open device: \"%s\"\n", pUsers[i].fileName );
      pUsers[i].fd = open( pUsers[i].fileName, O_RDONLY );
      if( pUsers[i].fd < 0 )
      {
         fprintf( stderr, "ERROR: Unable to open device: \"%s\"\n", pUsers[i].fileName );
         goto L_ERROR;
      }
      fdMax++;
      if( pUsers[i].fd > fdMax )
         fdMax = pUsers[i].fd;
      pUsers[i].recCount = 0;
   }
   fdMax++;

   fd_set rfds;
   int inKey = 0;
   do
   {
      FD_ZERO( &rfds );
      FD_SET( STDIN_FILENO, &rfds );
      for( int i = 0; i < numOfInstances; i++ )
      {
         if( pUsers[i].fd > 0 )
            FD_SET( pUsers[i].fd, &rfds );
      }

      int state = select( fdMax, &rfds, NULL, NULL, NULL );
      if( state < 0 )
         break;
      if( state == 0 )
         continue;

      for( int i = 0; i < numOfInstances; i++ )
      {
         if( (pUsers[i].fd > 0) && FD_ISSET( pUsers[i].fd, &rfds ))
         {
            readBytes = read( pUsers[i].fd, g_textBuffer, sizeof(g_textBuffer)-1 );
            if( readBytes < 0 )
            {
               fprintf( stderr, "ERROR: unable to read from \"%s\": %s\n",
                                pUsers[i].fileName,
                                strerror( errno ) );
               continue;
            }
            if( readBytes > 0 )
            {
               pUsers[i].recCount++;
               g_textBuffer[readBytes] = '\0';
               printf( ESC_XY ESC_CLR_LINE "Device: %s, count: %s received: %u\n", i+numOfInstances + 6, 1,  pUsers[i].fileName, g_textBuffer, pUsers[i].recCount );
            }
         }
      }

      if( FD_ISSET( STDIN_FILENO, &rfds ) )
      {
         if( read( STDIN_FILENO, &inKey, sizeof( inKey ) ) > 0 )
         {
            inKey &= 0xFF;
            if( inKey == '\e' )
            {
               printf( "Exit loop...\n" );
            }
            if( inKey == 's' )
            {
               printf( "sleeping...\r" );
               fflush( NULL );
               sleep( 1 );
               printf( ESC_CLR_LINE );
            }
         }
      }
   }
   while( inKey != '\e' );

L_ERROR:
   for( int i = 0; i < numOfInstances; i++ )
   {
      if( pUsers[i].fd < 0 )
         continue;
      printf( "Close device: \"%s\"\n", pUsers[i].fileName );
      close( pUsers[i].fd );
   }

   free( pUsers );
   resetTerminalInput();
   printf( "End...\n" );
   return EXIT_SUCCESS;
}
