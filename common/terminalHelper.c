/*****************************************************************************/
/*                                                                           */
/*! @brief Module for reading single key- hits                               */
/*                                                                           */
/*---------------------------------------------------------------------------*/
/*! @file   terminalHelper.c                                                 */
/*! @author Ulrich Becker                                                    */
/*! @date   26.03.2025                                                       */
/*****************************************************************************/
#include <termios.h>
#include <unistd.h>
#include "terminalHelper.h"


static struct termios g_originTerminal;

/*-----------------------------------------------------------------------------
*/
int prepareTerminalInput( void )
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

/*-----------------------------------------------------------------------------
*/
int resetTerminalInput( void )
{
   return tcsetattr( STDIN_FILENO, TCSANOW, &g_originTerminal );
}

/*================================== EOF ====================================*/
