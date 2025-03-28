/*****************************************************************************/
/*                                                                           */
/*! @brief Module for reading single key- hits                               */
/*                                                                           */
/*---------------------------------------------------------------------------*/
/*! @file   terminalHelper.h                                                 */
/*! @author Ulrich Becker                                                    */
/*! @date   26.03.2025                                                       */
/*****************************************************************************/
#ifndef _TERMINALHELPER_H
#define _TERMINALHELPER_H

#ifdef __cplusplus
extern "C" {
#endif

#define _ESC_XY( _X, _Y ) "\e[" _Y ";" _X "H"
#define ESC_XY _ESC_XY( "%u", "%u" )
#define ESC_CLR_LINE  "\e[K"
#define ESC_CLR_SCR   "\e[2J"


int prepareTerminalInput( void );

int resetTerminalInput( void );

#ifdef __cplusplus
}
#endif
#endif /* _TERMINALHELPER_H */
/*================================== EOF ====================================*/
