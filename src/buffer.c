/*
 * Buffer, implemented as Single-Writer Single-Reader circular buffer.
 *
 * It uses ideas from the FastForward queue implementation:
 * Synchronisation takes place with the content of the buffer, i.e., NULL indicates that
 * the location is empty. After reading a value, the consumer writes NULL back to the
 * position it read the data from.
 *
 * @see http://www.cs.colorado.edu/department/publications/reports/docs/CU-CS-1023-07.pdf
 *      accessed Aug 26, 2010
 *      for more details on the FastForward queue.
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>


#include "buffer.h"


/**
 * Initialize a buffer.
 * Also allocates space for size void* items
 *
 * @param buf   pointer to buffer struct
 * @param size  number of void* elements in the buffer
 */
void LpelBufferInit( buffer_t *buf, unsigned int size)
{
  buf->pread = 0;
  buf->pwrite = 0;
  buf->size = size;
  buf->data = malloc( size*sizeof(void*) );
  /* clear all the buffer space */
  memset(buf->data, 0, size*sizeof(void *));
}

/**
 * Cleanup the buffer.
 * Free the memory for the buffer items.
 *
 * @param buf   pointer to buffer struct
 */
void  LpelBufferCleanup(buffer_t *buf)
{
  free(buf->data);
}


/**
 * Returns the top from a buffer
 *
 * @pre         no concurrent reads
 * @param buf   buffer to read from
 * @return      NULL if buffer is empty
 */
void *LpelBufferTop( buffer_t *buf)
{
  /* if the buffer is empty, data[pread]==NULL */
  return buf->data[buf->pread];
}


/**
 * Consuming read from a stream
 *
 * Implementation note:
 * - modifies only pread pointer (not pwrite)
 *
 * @pre         no concurrent reads
 * @param buf   buffer to read from
 */
void LpelBufferPop( buffer_t *buf)
{
  /* clear, and advance pread */
  buf->data[buf->pread]=NULL;
  buf->pread += (buf->pread+1 >= buf->size) ? (1-buf->size) : 1;
}


/**
 * Check if there is space in the buffer
 *
 * @param buf   buffer to check
 * @pre         no concurrent calls
 */
int LpelBufferIsSpace( buffer_t *buf)
{
  /* if there is space in the buffer, the location at pwrite holds NULL */
  return ( buf->data[buf->pwrite] == NULL );
}


/**
 * Put an item into the buffer
 *
 * Precondition: item != NULL
 *
 * Implementation note:
 * - modifies only pwrite pointer (not pread)
 *
 * @param buf   buffer to write to
 * @param item  data item (a pointer) to write
 * @pre         no concurrent writes
 * @pre         item != NULL
 * @pre         there has to be space in the buffer
 *              (check with BufferIsSpace)
 */
void LpelBufferPut( buffer_t *buf, void *item)
{
  assert( item != NULL );
  assert( LpelBufferIsSpace(buf) );

  /* WRITE TO BUFFER */
  /* Write Memory Barrier: ensure all previous memory write
   * are visible to the other processors before any later
   * writes are executed.  This is an "expensive" memory fence
   * operation needed in all the architectures with a weak-ordering
   * memory model where stores can be executed out-or-order
   * (e.g. PowerPC). This is a no-op on Intel x86/x86-64 CPUs.
   */
  WMB();
  buf->data[buf->pwrite] = item;
  buf->pwrite += (buf->pwrite+1 >= buf->size) ? (1-buf->size) : 1;
}




