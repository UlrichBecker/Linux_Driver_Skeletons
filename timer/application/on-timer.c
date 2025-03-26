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
#include <dirent.h>

#ifndef ARRAY_SIZE
 #define ARRAY_SIZE( a ) (sizeof( a ) / sizeof( a[0] ))
#endif

typedef struct
{
   char  fileName[16];
   int   fd;
} POLL_OBJ_T;

static struct termios g_originTerminal;

/*-----------------------------------------------------------------------------
*/
static inline int resetTerminalInput( void )
{
   return tcsetattr( STDIN_FILENO, TCSANOW, &g_originTerminal );
}

/*-----------------------------------------------------------------------------
*/
static int prepareTerminalInput( void )
{
   struct termios newTerminal;

   if( tcgetattr( STDIN_FILENO, &g_originTerminal ) < 0 )
      return -1;

   newTerminal = g_originTerminal;
   newTerminal.c_lflag     &= ~(ICANON | ECHO);  /* Disable canonic mode and echo.*/
   newTerminal.c_cc[VMIN]  = 1;  /* Reading is complete after one byte only. */
   newTerminal.c_cc[VTIME] = 0;  /* No timer. */

   return tcsetattr( STDIN_FILENO, TCSANOW, &newTerminal );
}


static int getNumberOfFoundDriverInstances( void )
{
   DIR*           pDir;
   struct dirent* poEntry;
   int count = 0;

   pDir = opendir( "/dev" );
   if( pDir == NULL )
      return -1;

   while( (poEntry = readdir( pDir )) != NULL )
   {
      if( poEntry->d_type == DT_DIR )
         continue;

      if( strncmp( "timer", poEntry->d_name, 5 ) != 0 )
         continue;

      count++;
   }
   closedir( pDir );
   return count;
}

int main( void )
{
   printf( "Test of Linux-kernel-driver \"timer\"\n" );

   const int numOfInstances = getNumberOfFoundDriverInstances();
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
   {
      fprintf( stderr, "ERROR Unable to prepare termilal input!\n" );
      return EXIT_FAILURE;
   }

   int fdMax = STDIN_FILENO;
   for( int i = 0; i < numOfInstances; i++ )
   {
      snprintf( pUsers[i].fileName, ARRAY_SIZE( pUsers[0].fileName ), "/dev/timer%d", i );
      printf( "Open device: \"%s\"\n", pUsers[i].fileName );
      pUsers[i].fd = open( pUsers[i].fileName, O_RDONLY );
      if( pUsers[i].fd < 0 )
      {
         fprintf( stderr, "ERROR: Unable to open device: \"%s\"\n", pUsers[i].fileName );
         goto L_ERROR;
      }
      fdMax++;
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
         //TODO
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
