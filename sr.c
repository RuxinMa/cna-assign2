#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "emulator.h"
#include "sr.h"

/* ******************************************************************
   Selective Repeat protocol.  Adapted from J.F.Kurose
   ALTERNATING BIT AND GO-BACK-N NETWORK EMULATOR: VERSION 1.2  

   Network properties:
   - one way network delay averages five time units (longer if there
   are other messages in the channel for GBN), but can be larger
   - packets can be corrupted (either the header or the data portion)
   or lost, according to user-defined probabilities
   - packets will be delivered in the order in which they were sent
   (although some can be lost).

   Modifications: 
   - Converted from Go-Back-N to Selective Repeat
**********************************************************************/

#define RTT  16.0       /* round trip time.  MUST BE SET TO 16.0 when submitting assignment */
#define WINDOWSIZE 6    /* the maximum number of buffered unacked packet */
#define SEQSPACE 12     /* the min sequence space for GBN must be at least windowsize + 1 */
#define NOTINUSE (-1)   /* used to fill header fields that are not being used */
#define MAX_RETRANSMISSIONS 10  /* Maximum number of retransmission attempts before assuming packet was received */ 

/* Array to track retransmission counts for each packet in the window */ 
static int retransmission_count[WINDOWSIZE] = {0};

/* generic procedure to compute the checksum of a packet.  Used by both sender and receiver  
   the simulator will overwrite part of your packet with 'z's.  It will not overwrite your 
   original checksum.  This procedure must generate a different checksum to the original if
   the packet is corrupted.
*/
int ComputeChecksum(struct pkt packet)
{
  int checksum = 0;
  int i;

  checksum = packet.seqnum;
  checksum += packet.acknum;
  for ( i=0; i<20; i++ ) 
    checksum += (int)(packet.payload[i]);

  return checksum;
}

bool IsCorrupted(struct pkt packet)
{
  if (packet.checksum == ComputeChecksum(packet))
    return (false);
  else
    return (true);
}


/********* Sender (A) variables and functions ************/

/* Selective Repeat data structures for sender */
typedef enum {
    UNUSED,        /* empty slot in send window */
    SENT,          /* packet sent, waiting for ACK */
    ACKED          /* packet acknowledged */
} packet_status;

static struct pkt send_buffer[WINDOWSIZE];      /* array for storing packets */
static packet_status send_status[WINDOWSIZE];  /* status of each packet */
static double send_time[WINDOWSIZE];          /* time when packet was sent */
static int send_base;                         /* sequence number of first unACKed packet */
static int next_seqnum;                      /* next sequence number to use */

/* Helper function to translate sequence number to buffer index */
static int seq_to_index(int seqnum)
{
    return seqnum % WINDOWSIZE;
}

/* Helper function to check if seqnum is in send window */
static bool in_send_window(int seqnum)
{
    int window_end = (send_base + WINDOWSIZE - 1) % SEQSPACE;
    
    if (send_base <= window_end) {
        /* Normal case: window doesn't wrap around */
        return (seqnum >= send_base && seqnum <= window_end);
    } else {
        /* window wraps around */
        return (seqnum >= send_base || seqnum <= window_end);
    }
}

/* Helper function to check if any packet needs a timer running */
static bool has_running_timer(void)
{
    int i;
    for (i = 0; i < WINDOWSIZE; i++) {
        if (send_status[i] == SENT) {
            return true;
        }
    }
    return false;
}

/* called from layer 5 (application layer), passed the message to be sent to other side */
void A_output(struct msg message)
{
  struct pkt sendpkt;
  int i;
  int index;
  extern float time;  /* get current time from emulator */

  /* check if we can send a new packet */
  if (in_send_window(next_seqnum)) {
    if (TRACE > 1)
      printf("----A: New message arrives, send window is not full, send new messge to layer3!\n");

    /* create packet */
    sendpkt.seqnum = next_seqnum;
    sendpkt.acknum = NOTINUSE;
    for ( i=0; i<20 ; i++ ) 
      sendpkt.payload[i] = message.data[i];
    sendpkt.checksum = ComputeChecksum(sendpkt); 

    /* store packet in send buffer */
    index = seq_to_index(next_seqnum);
    send_buffer[index] = sendpkt;
    send_status[index] = SENT;
    retransmission_count[index] = 0;  /* Reset retransmission counter for new packet */ 
    send_time[index] = time;  /* record when packet was sent */

    /* send out packet */
    if (TRACE > 0)
      printf("Sending packet %d to layer 3\n", sendpkt.seqnum);
    tolayer3 (A, sendpkt);

    /* if no timer is running, start one */
    if (!has_running_timer())
      starttimer(A,RTT);

    /* get next sequence number, wrap back to 0 */
    next_seqnum = (next_seqnum + 1) % SEQSPACE;  
  }
  /* if blocked,  window is full */
  else {
    if (TRACE > 0)
      printf("----A: New message arrives, send window is full\n");
    window_full++;
  }
}


/* called from layer 3, when a packet arrives for layer 4 
   In this practical this will always be an ACK as B never sends data.
*/
void A_input(struct pkt packet)
{
  /* if received ACK is not corrupted */ 
  if (!IsCorrupted(packet)) {
    if (TRACE > 0)
      printf("----A: uncorrupted ACK %d is received\n",packet.acknum);
    total_ACKs_received++;

    int index = seq_to_index(packet.acknum);
    
    if (in_send_window(packet.acknum) || 
        packet.acknum == ((send_base + WINDOWSIZE - 1) % SEQSPACE) ||
        packet.acknum == ((send_base - 1 + SEQSPACE) % SEQSPACE)) {

      /* Check if this packet hasn't been ACKed yet */
      if (send_status[index] == SENT) {
        /* Mark packet as acknowledged */
        send_status[index] = ACKED;
        retransmission_count[index] = 0;  /* Reset retransmission counter */
          
        if (TRACE > 0)
          printf("----A: ACK %d is new, marked as ACKED\n",packet.acknum);
        new_ACKs++;

        /* Check if we can slide the window */
        while (send_status[seq_to_index(send_base)] == ACKED) {
          /* Mark slot as unused */
          send_status[seq_to_index(send_base)] = UNUSED;
          /* Slide window by one */
          send_base = (send_base + 1) % SEQSPACE;
          if (TRACE > 0)
            printf("----A: Window slides to %d\n", send_base);
        }

        /* start timer again if there are still more unacked packets in window */
        stoptimer(A);
        if (has_running_timer())
          starttimer(A, RTT);
      }
      else {
        if (TRACE > 0)
          printf ("----A: duplicate ACK received, do nothing!\n");
      }
    }
    /* Handle duplicate ACKs (ACKs before the window) */
    else if  (((packet.acknum < send_base) && 
              ((send_base - packet.acknum) <= WINDOWSIZE)) || 
             ((packet.acknum > send_base) && 
              ((packet.acknum - send_base) > (SEQSPACE - WINDOWSIZE)))) {
      if (TRACE > 0)
        printf("----A: duplicate ACK for packet before window, ignoring\n");
      /* This is an ACK for a packet already processed, ignore it */
    } 
    else {
      if (TRACE > 0)
        printf("----A: ACK %d outside window, do nothing!\n", packet.acknum);
    }
  }
  else {
    if (TRACE > 0)
      printf ("----A: corrupted ACK is received, do nothing!\n");
  }
}

/* called when A's timer goes off */
void A_timerinterrupt(void)
{
  int i;
  int resent = 0;
  extern float time;  /* get current time from emulator */

  if (TRACE > 0)
    printf("----A: time out,resend packets!\n");
  
  /* Resend all unacknowledged packets in window */
  for(i = 0; i < WINDOWSIZE; i++) {
    int seqnum = (send_base + i) % SEQSPACE;
    
    /* Ensure we only resend packets within the current window */
    if (in_send_window(seqnum)){
      int index = seq_to_index(seqnum);

      if (send_status[index] == SENT) {
        /*  Check if we've reached the retransmission limit */
        if (retransmission_count[index] < MAX_RETRANSMISSIONS) {
          if (TRACE > 0)
            printf("---A: resending packet %d (retransmission %d)\n", 
                   seqnum, retransmission_count[index] + 1);

        tolayer3(A, send_buffer[index]);
        packets_resent++;
        resent = 1;

        /* Update time when packet was sent */
        send_time[index] = time;
        /* Increment retransmission counter */
        retransmission_count[index]++;
      } else {
          /* Max retransmissions reached, mark as ACKED to avoid deadlock */ 
          if (TRACE > 0)
            printf("---A: packet %d reached max retransmissions, marking as ACKED\n", seqnum);
          
          send_status[index] = ACKED;

          if (seqnum == send_base) {
            while (send_status[seq_to_index(send_base)] == ACKED) {
              /* Mark slot as unused */
              send_status[seq_to_index(send_base)] = UNUSED;
              /* Slide window by one */
              send_base = (send_base + 1) % SEQSPACE;
              if (TRACE > 0)
                printf("----A: Window slides to %d\n", send_base);
            }
          }
        }
      }
    }
  }

  /* Check if we can slide the window after forcing ACKs */
  while (send_status[seq_to_index(send_base)] == ACKED) {
    /* Mark slot as unused and reset retransmission counter */
    int index = seq_to_index(send_base);
    send_status[index] = UNUSED;
    retransmission_count[index] = 0;
    
    /* Slide window by one */
    send_base = (send_base + 1) % SEQSPACE;
    if (TRACE > 0)
      printf("----A: Window slides to %d\n", send_base);
  }

  /* Reset timer unless there are no packets to resend */
  if (resent) {
    starttimer(A,RTT);
  } else {
    if (TRACE > 0)
      printf("----A: No packets in window to resend, window may be empty\n");
    /* If no packets need to be resent, window may be empty,
       or all packets already ACKed, no need to restart timer */
  }
}       



/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init(void)
{
  int i;
    
    /* Initialize sender state */
    send_base = 0;
    next_seqnum = 0;
    
    /* Initialize send buffer and status */
    for (i = 0; i < WINDOWSIZE; i++) {
        send_status[i] = UNUSED;
        /* Initialize retransmission counters */
        retransmission_count[i] = 0;  
    }
}



/********* Receiver (B)  variables and procedures ************/

/* Selective Repeat data structures for receiver */
static struct pkt recv_buffer[WINDOWSIZE];      /* buffer for out-of-order packets */
static bool recv_status[WINDOWSIZE];           /* status for each packet in window */
static int recv_base;                          /* lowest sequence number in window */
static int B_nextseqnum;                       /* sequence number for ACK packets */

/* Helper function to check if seqnum is in receive window */
static bool in_recv_window(int seqnum)
{
    int window_end = (recv_base + WINDOWSIZE - 1) % SEQSPACE;
    
    if (recv_base <= window_end) {
        /* Normal case: window doesn't wrap around */
        return (seqnum >= recv_base && seqnum <= window_end);
    } else {
        /* window wraps around */
        return (seqnum >= recv_base || seqnum <= window_end);
    }
}

/* Helper function to translate sequence number to buffer index */
static int recv_seq_to_index(int seqnum)
{
    return seqnum % WINDOWSIZE;
}

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(struct pkt packet)
{
  struct pkt sendpkt;
  int i;

  /* Check if packet is corrupted */
  if  ( IsCorrupted(packet) ) {
    if (TRACE > 0)
      printf("----B: packet %d is corrupted\n",packet.seqnum);
    
    /* Send ACK for last correctly received packet */
    sendpkt.acknum = (recv_base - 1 + SEQSPACE) % SEQSPACE;
  }
  else {
    if (TRACE > 0) 
      printf("----B: packet %d is not corrupted\n", packet.seqnum);

    /* Check if packet is within our receive window */
    if (in_recv_window(packet.seqnum)) {
      int index = recv_seq_to_index(packet.seqnum);
      
      /* If we haven't received this packet before */
      if (!recv_status[index]) {
        /* Store packet in buffer */
        recv_buffer[index] = packet;
        recv_status[index] = true;
        
        if (TRACE > 0)
          printf("----B: packet %d accepted and buffered\n", packet.seqnum);
        
        /* If this is the packet we're waiting for, deliver it and any consecutive buffered packets */
        if (packet.seqnum == recv_base) {
          while (recv_status[recv_seq_to_index(recv_base)]) {
            /* Deliver packet to layer 5 */
            index = recv_seq_to_index(recv_base);
            tolayer5(B, recv_buffer[index].payload);
            packets_received++;
            
            /* Mark buffer slot as empty */
            recv_status[index] = false;
            
            /* Advance receive window */
            recv_base = (recv_base + 1) % SEQSPACE;
            
            if (TRACE > 0)
              printf("----B: delivered packet to layer 5, window moves to %d\n", recv_base);
          }
        }
      } else {
        if (TRACE > 0)
          printf("----B: duplicate packet %d received, already buffered\n", packet.seqnum);
      }
        
      /* Send ACK for this packet */
      sendpkt.acknum = packet.seqnum;
    }
    else {
      if (TRACE > 0)
        printf("----B: packet %d outside window\n", packet.seqnum);
    
      /* If packet is a duplicate (before our window) */
      if (((packet.seqnum < recv_base) && ((recv_base - packet.seqnum) <= WINDOWSIZE)) || 
          ((packet.seqnum > recv_base) && ((packet.seqnum - recv_base) > (SEQSPACE - WINDOWSIZE)))) {
        /* Send ACK for this packet as it was previously received */
        sendpkt.acknum = packet.seqnum;
        if (TRACE > 0)
          printf("----B: duplicate packet, sending ACK %d\n", sendpkt.acknum);
      }
      else {
        /* Otherwise, send ACK for last correctly received packet */
        sendpkt.acknum = (recv_base - 1 + SEQSPACE) % SEQSPACE;
        if (TRACE > 0)
          printf("----B: sending ACK for last correctly received packet %d\n", sendpkt.acknum);
      }
    }
  }

  /* create packet */
  sendpkt.seqnum = B_nextseqnum;
  B_nextseqnum = (B_nextseqnum + 1) % 2;
    
  /* we don't have any data to send.  fill payload with 0's */
  for (i = 0; i < 20 ; i++) 
    sendpkt.payload[i] = '0';  

  /* computer checksum */
  sendpkt.checksum = ComputeChecksum(sendpkt); 

  /* send out packet */
  tolayer3 (B, sendpkt);
}

/* the following routine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init(void)
{
  int i;

  /* Initialize receiver variables */
  recv_base = 0;
  B_nextseqnum = 1;

  /* Initialize receiver buffer */
    for (i = 0; i < WINDOWSIZE; i++) {
        recv_status[i] = false;
    }
}

/******************************************************************************
 * The following functions need be completed only for bi-directional messages *
 *****************************************************************************/

/* Note that with simplex transfer from a-to-B, there is no B_output() */
void B_output(struct msg message)  
{
}

/* called when B's timer goes off */
void B_timerinterrupt(void)
{
}

