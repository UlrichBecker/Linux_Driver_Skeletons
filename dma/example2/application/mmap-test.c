/*****************************************************************************/
/*                                                                           */
/*! @brief Application part in user-space to demonstrate how cooperates the  */
/*         function select() with a kernel-module.                           */
/*                                                                           */
/*---------------------------------------------------------------------------*/
/*! @file   mmap-test.c                                                      */
/*! @author Ulrich Becker                                                    */
/*! @date   10.04.2025                                                       */
/*****************************************************************************/
#include <stdio.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/mman.h>


#define DRIVER_NAME "/dev/dmatest_user"
#define SIZE 4096

int main( void )
{
   printf( "Applicatuon part for testing the demo driver \"dmatest-user\"\n" );

   int fd = open( DRIVER_NAME, O_RDWR );
   if( fd < 0 )
   {
      fprintf( stderr, "Can't open: " DRIVER_NAME "\n" );
      return EXIT_FAILURE;
   }

   void* pMem = mmap( NULL, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0 );
   if( pMem == MAP_FAILED )
   {
      fprintf( stderr, "Can't make memory-map\n" );
      close( fd );
   }

   strcpy( pMem, "Hello DMA!");
   printf("DMA-Buffer-Content: %s\n", (char*)pMem );

   munmap( pMem, SIZE );
   close( fd );
   return EXIT_SUCCESS;
}

/*================================== EOF ====================================*/
