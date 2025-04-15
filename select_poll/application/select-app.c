/*****************************************************************************/
/*                                                                           */
/*! @brief Application part in user-space to demonstrate how cooperates the  */
/*         function select() with a kernel-module.                           */
/*                                                                           */
/*---------------------------------------------------------------------------*/
/*! @file   select-app.c                                                     */
/*! @author Ulrich Becker                                                    */
/*! @date   05.10.2017                                                       */
/*****************************************************************************/
/*! @note It's possible to run this program as non-root user if you
 *        creates a new udev-rule:
 *! @code
 * KERNEL=="poll[0-9]", MODE="0666"
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
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <findInstances.h>
#include <terminalHelper.h>
#ifndef ARRAY_SIZE
 #define ARRAY_SIZE( a ) (sizeof( a ) / sizeof( a[0] ))
#endif

#define BASE_NAME "poll"

typedef struct
{
   char  fileName[16];
   int   fd;
} POLL_OBJ_T;

char g_Buffer[1024];

/*===========================================================================*/
int main( void )
{
   int fdMax = STDIN_FILENO;
   int state;
   int i;
   ssize_t readBytes;

   printf( "Poll-Test. Hit Esc to end.\n"
           "Open a further console and send a message to /dev/" BASE_NAME " or /dev/" BASE_NAME "\n"
           "E.g.: echo \"Hello world\" > /dev/" BASE_NAME "\n" );

   const int numOfInstances = getNumberOfFoundDriverInstances( BASE_NAME );
   if( numOfInstances < 0 )
   {
      fprintf( stderr, "ERROR: Directory not found!\n" );
      return EXIT_FAILURE;
   }
   printf( "Found driver instances: %d\n", numOfInstances );
   if( numOfInstances == 0 )
   {
      printf( "No driver-instance found.\n" );
      return EXIT_SUCCESS;
   }

   POLL_OBJ_T* pUsers = malloc( numOfInstances * sizeof(POLL_OBJ_T) );
   if( pUsers == NULL )
   {
      fprintf( stderr, "ERROR: Unable to allocate memory for %d instances!\n", numOfInstances );
      return EXIT_FAILURE;
   }

   if( prepareTerminalInput() != 0 )
      return EXIT_FAILURE;

   for( i = 0; i < numOfInstances; i++ )
   {
      snprintf( pUsers[i].fileName, ARRAY_SIZE( pUsers[0].fileName ), "/dev/" BASE_NAME "%d", i );
      printf( "Open device: \"%s\"\n", pUsers[i].fileName );
      pUsers[i].fd = open( pUsers[i].fileName, O_RDONLY );
      if( pUsers[i].fd < 0 )
      {
         fprintf( stderr, "ERROR: Unable to open device: \"%s\"\n", pUsers[i].fileName );
         goto L_ERROR;
      }
      if( pUsers[i].fd > fdMax )
         fdMax = pUsers[i].fd;
   }
   fdMax++;

   fd_set rfds;
   int inKey = 0;
   do
   {
      FD_ZERO( &rfds );
      FD_SET( STDIN_FILENO, &rfds );
      for( i = 0; i < numOfInstances; i++ )
      {
         if( pUsers[i].fd > 0 )
            FD_SET( pUsers[i].fd, &rfds );
      }
      state = select( fdMax, &rfds, NULL, NULL, NULL );
      if( state < 0 )
         break;
      if( state == 0 )
         continue;

      for( i = 0; i < numOfInstances; i++ )
      {
         if( (pUsers[i].fd > 0) && FD_ISSET( pUsers[i].fd, &rfds ))
         {
            readBytes = read( pUsers[i].fd, g_Buffer, sizeof(g_Buffer) );
            if( readBytes < 0 )
            {
               fprintf( stderr, "ERROR: unable to read from \"%s\": %s\n",
                                pUsers[i].fileName,
                                strerror( errno ) );
               continue;
            }
            if( readBytes > 0 )
            {
               printf( "%s: ", pUsers[i].fileName );
               fflush( NULL );
               write( STDOUT_FILENO, g_Buffer, readBytes );
               if( (readBytes > 1) && (g_Buffer[readBytes-1] != '\n') )
                  puts( "\n" );
               fflush( NULL );
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
               printf( "End...\n" );
            }
         }
      }
   }
   while( inKey != '\e' );

L_ERROR:
   for( i = 0; i < numOfInstances; i++ )
   {
      if( pUsers[i].fd < 0 )
         continue;
      printf( "Close device: \"%s\"\n", pUsers[i].fileName );
      close( pUsers[i].fd );
   }
   free( pUsers );
   resetTerminalInput();
   return EXIT_SUCCESS;
}

/*================================== EOF ====================================*/
