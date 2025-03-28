/*****************************************************************************/
/*                                                                           */
/*! @brief Module for finding kerner driver instances                        */
/*                                                                           */
/*---------------------------------------------------------------------------*/
/*! @file   findInstances.c                                                  */
/*! @author Ulrich Becker                                                    */
/*! @date   26.03.2025                                                       */
/*****************************************************************************/
#include <string.h>
#include <dirent.h>
#include <stdbool.h>
#include "findInstances.h"

/*-----------------------------------------------------------------------------
*/
static inline bool isDecDigit( char c )
{
   return (c >= '0') && (c <= '9');
}

/*-----------------------------------------------------------------------------
*/
int getNumberOfFoundDriverInstances( char* baseName )
{
   DIR*           pDir;
   struct dirent* poEntry;
   int count = 0;

   pDir = opendir( "/dev" );
   if( pDir == NULL )
      return -1;

   const size_t len = strlen( baseName );
   while( (poEntry = readdir( pDir )) != NULL )
   {
      if( poEntry->d_type != DT_CHR )
         continue;

      if( strncmp( baseName, poEntry->d_name, len ) != 0 )
         continue;

      if( !isDecDigit( poEntry->d_name[len] ) )
         continue;

      count++;
   }
   closedir( pDir );

   return count;
}

/*================================== EOF ====================================*/
