#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "lpel.h"
#include "arch/timing.h"

#define STACK_SIZE (16*1024) /* 16k */

#define PIPE_DEPTH 100 /* min 3*/

#define NUM_MSGS 100000L

#define MSG_TERM ((void*)-1)

typedef struct {
  lpel_stream_t *in, *out;
  int id;
} task_arg_t;

static timing_t ts;


void Source(lpel_task_t *self, void *inarg)
{
  unsigned long cnt = 0;
  lpel_stream_desc_t *out;
  void *msg;

  printf("Starting message transfer, pipe length %d msgs %lu\n", PIPE_DEPTH, NUM_MSGS);
  TimingStart( &ts);

  out = LpelStreamOpen( self, (lpel_stream_t *)inarg, 'w');

  while( cnt < (NUM_MSGS-1) ) {
    msg = (void*)(0x10000000 + cnt);
    LpelStreamWrite( out, msg);
    cnt++;
  }

  msg = MSG_TERM;
  LpelStreamWrite( out, msg);

  LpelStreamClose( out, 0);
}



void Sink(lpel_task_t *self, void *inarg)
{
  unsigned long cnt = 0;
  lpel_stream_desc_t *in;
  void *msg;

  in = LpelStreamOpen( self, (lpel_stream_t *)inarg, 'r');

  while(1) {
    msg = LpelStreamRead( in);
    cnt++;
    if (cnt % 10000 == 0) {
      //printf("Read %lu msgs\n", cnt);
    }
    if (msg==MSG_TERM) break;
  }

  LpelStreamClose( in, 1);

  TimingEnd( &ts);
  printf("End of message stream, cnt %lu duration %.2f ms\n",
      cnt, TimingToMSec(&ts));
}

void Relay(lpel_task_t *self, void *inarg)
{
  task_arg_t *arg = (task_arg_t *)inarg;
  //int id = arg->id;
  lpel_stream_desc_t *in, *out;
  int term = 0;
  void *msg;

  in = LpelStreamOpen( self, arg->in, 'r');
  out = LpelStreamOpen( self, arg->out, 'w');
  
  while(!term) {
    msg = LpelStreamRead( in);
    if (msg==MSG_TERM) term = 1;
    LpelStreamWrite( out, msg);
  }

  LpelStreamClose( in, 1);
  LpelStreamClose( out, 0);

  free(arg);

  LpelTaskExit(self, NULL);
}


static void CreateTask(task_arg_t *arg)
{
  lpel_taskreq_t *t;
  int flags = LPEL_TASK_ATTR_NONE;

  t = LpelTaskRequest( Relay, arg, flags, STACK_SIZE, 0);
  //LpelWorkerTaskAssign( t, 0);
  LpelWorkerTaskAssign( t, (arg->id < PIPE_DEPTH/2) ? 0 : 1);
}


lpel_stream_t *PipeElement(lpel_stream_t *in, int depth)
{
  lpel_stream_t *out;
  task_arg_t *arg;


  out = LpelStreamCreate();
  arg = (task_arg_t *) malloc( sizeof(task_arg_t));
  arg->in = in;
  arg->out = out;
  arg->id = depth;

  CreateTask(arg);

  //printf("Created Relay %d\n", depth );
  return (depth < (PIPE_DEPTH-2)) ? PipeElement( out, depth+1) : out;
}

static void CreatePipe(void)
{
  lpel_stream_t *glob_in, *glob_out;
  lpel_taskreq_t *t;
  int flags = LPEL_TASK_ATTR_NONE;


  glob_in = LpelStreamCreate();
  glob_out = PipeElement(glob_in, 1);

  t = LpelTaskRequest( Source, glob_in, flags, STACK_SIZE, 0);
  LpelWorkerTaskAssign( t, 0);

  t = LpelTaskRequest( Sink, glob_out, flags, STACK_SIZE, 0);
  LpelWorkerTaskAssign( t, 0);
}




static void testBasic(void)
{
  lpel_config_t cfg;

  cfg.num_workers = 2;
  cfg.proc_workers = 2;
  cfg.proc_others = 0;
  cfg.flags = 0;
  cfg.node = -1;

  
  LpelInit(&cfg);

  CreatePipe();


  LpelCleanup();
}




int main(void)
{
  testBasic();
  printf("test finished\n");
  return 0;
}