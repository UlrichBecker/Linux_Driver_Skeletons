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
#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#ifndef ARRAY_SIZE
 #define ARRAY_SIZE( a ) (sizeof( a ) / sizeof( a[0] ))
#endif



typedef struct
{
   const char* pFileName;
   int         fd;
} POLL_OBJ_T;


#define OBJ_INITIALIZER(n)         \
   [n] =                           \
   {                               \
      .pFileName = "/dev/poll" #n, \
      .fd = -1                     \
   }

POLL_OBJ_T g_pollObjs[2]=
{
   OBJ_INITIALIZER(0),
   OBJ_INITIALIZER(1)
};

char g_Buffer[1024];

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

/*===========================================================================*/
int main( void )
{
   int fdMax = STDIN_FILENO;
   fd_set rfds;
   int inKey = 0;
   int state;
   int i;
   ssize_t readBytes;

   printf( "Poll-Test. Hit Esc to end.\n" );
   if( prepareTerminalInput() != 0 )
      return EXIT_FAILURE;

   for( i = 0; i < ARRAY_SIZE(g_pollObjs); i++ )
   {
      g_pollObjs[i].fd = open( g_pollObjs[i].pFileName, O_RDONLY );
      if( g_pollObjs[i].fd > 0 )
      {
         if( g_pollObjs[i].fd > fdMax )
            fdMax = g_pollObjs[i].fd;
      }
      else
      {
         fprintf( stderr, "ERROR: Could not open \"%s\" : %s\n",
                          g_pollObjs[i].pFileName,
                          strerror( errno ) );
      }
   }
   fdMax++;
   do
   {
      FD_ZERO( &rfds );
      FD_SET( STDIN_FILENO, &rfds );
      for( i = 0; i < ARRAY_SIZE(g_pollObjs); i++ )
      {
         if( g_pollObjs[i].fd > 0 )
            FD_SET( g_pollObjs[i].fd, &rfds );
      }
      state = select( fdMax, &rfds, NULL, NULL, NULL );
      if( state < 0 )
         break;
      if( state == 0 )
         continue;

      for( i = 0; i < ARRAY_SIZE(g_pollObjs); i++ )
      {
         if( (g_pollObjs[i].fd > 0) && FD_ISSET( g_pollObjs[i].fd, &rfds ))
         {
            readBytes = read( g_pollObjs[i].fd, g_Buffer, sizeof(g_Buffer) );
            if( readBytes < 0 )
            {
               fprintf( stderr, "ERROR: unable to read from \"%s\": %s\n",
                                g_pollObjs[i].pFileName,
                                strerror( errno ) );
               continue;
            }
            if( readBytes > 0 )
            {
               printf( "%s: ", g_pollObjs[i].pFileName );
               fflush( NULL );
               write( STDOUT_FILENO, g_Buffer, readBytes );
              // if( (readBytes > 1) && (g_Buffer[readBytes-1] != '\n') )
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

   for( i = 0; i < ARRAY_SIZE(g_pollObjs); i++ )
   {
      if( g_pollObjs[i].fd > 0 )
         close( g_pollObjs[i].fd );
   } 
   resetTerminalInput();
   return EXIT_SUCCESS;
}

/*================================== EOF ====================================*/
