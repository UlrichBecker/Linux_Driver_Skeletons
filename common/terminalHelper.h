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

#define ESC_FG_BLACK   "\e[30m" /*!< @brief Foreground color black   */
#define ESC_FG_RED     "\e[31m" /*!< @brief Foreground color red     */
#define ESC_FG_GREEN   "\e[32m" /*!< @brief Foreground color green   */
#define ESC_FG_YELLOW  "\e[33m" /*!< @brief Foreground color yellow  */
#define ESC_FG_BLUE    "\e[34m" /*!< @brief Foreground color blue    */
#define ESC_FG_MAGENTA "\e[35m" /*!< @brief Foreground color magenta */
#define ESC_FG_CYAN    "\e[36m" /*!< @brief Foreground color cyan    */
#define ESC_FG_WHITE   "\e[37m" /*!< @brief Foreground color white   */

#define ESC_BG_BLACK   "\e[40m" /*!< @brief Background color black   */
#define ESC_BG_RED     "\e[41m" /*!< @brief Background color red     */
#define ESC_BG_GREEN   "\e[42m" /*!< @brief Background color green   */
#define ESC_BG_YELLOW  "\e[43m" /*!< @brief Background color yellow  */
#define ESC_BG_BLUE    "\e[44m" /*!< @brief Background color blue    */
#define ESC_BG_MAGENTA "\e[45m" /*!< @brief Background color magenta */
#define ESC_BG_CYAN    "\e[46m" /*!< @brief Background color cyan    */
#define ESC_BG_WHITE   "\e[47m" /*!< @brief Background color white   */

#define ESC_BOLD      "\e[1m"   /*!< @brief Bold on  */
#define ESC_BLINK     "\e[5m"   /*!< @brief Blink on */
#define ESC_NORMAL    "\e[0m"   /*!< @brief All attributes off */
#define ESC_HIDDEN    "\e[8m"   /*!< @brief Hidden on */

#define ESC_CLR_LINE  "\e[K"    /*!< @brief Clears the sctual line   */
#define ESC_CLR_SCR   "\e[2J"   /*!< @brief Clears the terminal screen */

#define ESC_CURSOR_OFF "\e[?25l" /*!< @brief Hides the cursor */
#define ESC_CURSOR_ON  "\e[?25h" /*!< @brief Restores the cursor */


#define ESC_ERROR   ESC_BOLD ESC_FG_RED    /*!< @brief Format for error messages */
#define ESC_WARNING ESC_BOLD ESC_FG_YELLOW /*!< @brief Format for warning messages */
#define ESC_DEBUG   ESC_FG_YELLOW          /*!< @brief Format for debug messages */


#define _ESC_XY( _X, _Y ) "\e[" _Y ";" _X "H"
#define ESC_XY _ESC_XY( "%u", "%u" )


int prepareTerminalInput( void );

int resetTerminalInput( void );

#ifdef __cplusplus
}
#endif
#endif /* _TERMINALHELPER_H */
/*================================== EOF ====================================*/
