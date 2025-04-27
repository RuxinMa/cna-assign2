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
#define SEQSPACE 12     /* the min sequence space for SR must be at least 2*WINDOWSIZE */
#define NOTINUSE (-1)   /* used to fill header fields that are not being used */
#define MAX_RETRANSMIT 30  /* Maximum retransmissions per packet before giving up */

/* generic procedure to compute the checksum of a packet.  Used by both sender and receiver  
   the simulator will overwrite part of your packet with 'z's.  It will not overwrite your 
   original checksum.  This procedure must generate a different checksum to the original if
   the packet is corrupted.
*/

/* Array to track retransmission counts for each packet in the window */ 
static int retransmission_count[WINDOWSIZE] = {0};
static int last_ack_sent = -1;  /* Last ACK number that was sent by receiver */
static int timer_seq = 0;       /* Track which packet the timer is set for */
static bool timer_running = false;

int ComputeChecksum(struct pkt packet)
{
  int checksum = 0;
  int i;

  checksum = packet.seqnum;
  checksum += packet.acknum;
  for (i = 0; i < 20; i++) 
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

/* Helper functions for timer management */ 
static void safe_start_timer(int entity, float increment) {
    if (!timer_running) {
        starttimer(entity, increment);
        timer_running = true;
    }
}

static void safe_stop_timer(int entity) {
    if (timer_running) {
        stoptimer(entity);
        timer_running = false;
    }
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
    int window_end;

    window_end = (send_base + WINDOWSIZE - 1) % SEQSPACE;
    
    if (send_base <= window_end) {
        /* Normal case: window doesn't wrap around */
        return (seqnum >= send_base && seqnum <= window_end);
    } else {
        /* window wraps around */
        return (seqnum >= send_base || seqnum <= window_end);
    }
}

/* Called to mark a packet as delivered if it's been retransmitted too many times */
static void advance_window_if_needed()
{
  /* If the base packet has been retransmitted too many times, mark it as delivered and advance window */
  while (send_base != next_seqnum) {
    int index = seq_to_index(send_base);
    if (send_status[index] == SENT && retransmission_count[index] >= MAX_RETRANSMIT) {
      if (TRACE > 0) {
        printf("----A: Packet %d exceeded max retransmissions, marking as delivered\n", send_base);
      }
      send_status[index] = ACKED;
      /* Continue the check by looking at next base */
      send_base = (send_base + 1) % SEQSPACE;
    } else {
      break;
    }
  }
}

/* called from layer 5 (application layer), passed the message to be sent to other side */
void A_output(struct msg message)
{
  struct pkt sendpkt;
  int i;
  int index;

  /* Check if we need to advance the window due to too many retransmissions */
  advance_window_if_needed();

  /* check if we can send a new packet */
  if (in_send_window(next_seqnum)) {
    if (TRACE > 1)
      printf("----A: New message arrives, send window is not full, send new messge to layer3!\n");

    /* create packet */
    sendpkt.seqnum = next_seqnum;
    sendpkt.acknum = NOTINUSE;
    for (i = 0; i < 20 ; i++) 
      sendpkt.payload[i] = message.data[i];
    sendpkt.checksum = ComputeChecksum(sendpkt); 

    /* store packet in send buffer */
    index = seq_to_index(next_seqnum);
    send_buffer[index] = sendpkt;
    send_status[index] = SENT;
    retransmission_count[index] = 0;  /* Reset retransmission counter for new packet */

    /* send out packet */
    if (TRACE > 0)
      printf("Sending packet %d to layer 3\n", sendpkt.seqnum);
    tolayer3 (A, sendpkt);
    
    /* Start timer if this is the first packet in the window */
    if (send_base == next_seqnum) {
      safe_start_timer(A, RTT);
      /* Record the serial number corresponding to the timer */
      timer_seq = send_base;
    }

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
  int index;
  bool need_restart_timer = false;
  bool was_timing_base = (timer_seq == send_base);

  /* if received ACK is not corrupted */ 
  if (!IsCorrupted(packet)) {
    if (TRACE > 0)
      printf("----A: uncorrupted ACK %d is received\n",packet.acknum);

    /* Check if the ACK is for a packet in our send window or the last acknowledged packet */
    if (in_send_window(packet.acknum) || 
        packet.acknum == ((send_base - 1 + SEQSPACE) % SEQSPACE)) {
    
      if (in_send_window(packet.acknum)) {
        index = seq_to_index(packet.acknum);
      
        /* Check if this packet hasn't been ACKed yet */
        if (send_status[index] == SENT) {
          /* Mark packet as acknowledged */
          send_status[index] = ACKED;
          retransmission_count[index] = 0;  /* Reset retransmission counter */
          
          if (TRACE > 0)
            printf("----A: ACK %d is not a duplicate\n",packet.acknum);
          new_ACKs++;

          /* If we're acknowledging the packet that the timer is for, we'll need to restart the timer */
          if (packet.acknum == timer_seq) {
            need_restart_timer = true;
            safe_stop_timer(A);
          }
        } else {
          /* ACK for packet right before window */
          if (TRACE > 0)
            printf("----A: ACK %d is a duplicate (for packet before window)\n", packet.acknum);
        }

        /* If this ACK is for the base packet or we need to restart timer */
        if (packet.acknum == send_base || need_restart_timer || was_timing_base) {
          /* Slide window over all consecutively ACKed packets */
          while (send_base != next_seqnum && 
                 send_status[seq_to_index(send_base)] == ACKED) {
            /* Mark slot as unused */
            send_status[seq_to_index(send_base)] = UNUSED;
            /* Slide window by one */
            send_base = (send_base + 1) % SEQSPACE;
          }

          /* Check again if we need to advance window due to max retransmissions */
          advance_window_if_needed();

          /* If we need to restart the timer and there are unacked packets */
          if (need_restart_timer || was_timing_base) {
            /* Find the first unacked packet */
            if (send_base != next_seqnum) {
              int first_unacked = send_base;
              while (first_unacked != next_seqnum && 
                    send_status[seq_to_index(first_unacked)] == ACKED) {
                first_unacked = (first_unacked + 1) % SEQSPACE;
              }

              if (first_unacked != next_seqnum) {
                timer_seq = first_unacked;
                safe_start_timer(A, RTT);
              }
            }
          }
        }
      } else {
        if (TRACE > 0)
          printf ("----A: ACK %d is a duplicate\n", packet.acknum);
      }
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
  int index;
  
  if (TRACE > 0)
    printf("----A: time out,resend packets!\n");

  timer_running = false; /* Reset timer state */ 
  
  /* Only process if there are unacked packets */
  if (send_base != next_seqnum) {
    index = seq_to_index(timer_seq);

    if (send_status[index] == SENT) {
      /* Only retransmit if we haven't reached max retransmissions */
      if (retransmission_count[index] < MAX_RETRANSMIT) {
        if (TRACE > 0)
          printf("---A: resending packet %d\n", timer_seq);
        
        tolayer3(A, send_buffer[index]); 
        packets_resent++;
        retransmission_count[index]++;

        /* Restart timer for this packet */
        safe_start_timer(A, RTT);
      } else {
        if (TRACE > 0)
          printf("---A: packet %d has reached max retransmissions (%d)\n", timer_seq, retransmission_count[index]);

        /* Mark as ACKed to allow window to advance */
        send_status[index] = ACKED;

        /* Slide window over all consecutively ACKed packets */
        while (send_base != next_seqnum && 
              send_status[seq_to_index(send_base)] == ACKED) {
          /* Mark slot as unused */
          send_status[seq_to_index(send_base)] = UNUSED;
          /* Slide window by one */
          send_base = (send_base + 1) % SEQSPACE;
        }

        /* Start timer for next unacked packet if any */
        if (send_base != next_seqnum) {
          /* Find the first unacked packet */
          int first_unacked = send_base;
          while (first_unacked != next_seqnum && 
                send_status[seq_to_index(first_unacked)] != SENT) {
            first_unacked = (first_unacked + 1) % SEQSPACE;
          }
          
          if (first_unacked != next_seqnum) {
            timer_seq = first_unacked;
            safe_start_timer(A, RTT);
          }
        }
      }
    } else {
      /* Find next unacked packet to time */
      if (send_base != next_seqnum) {
        int first_unacked = send_base;
        while (first_unacked != next_seqnum && 
              send_status[seq_to_index(first_unacked)] != SENT) {
          first_unacked = (first_unacked + 1) % SEQSPACE;
        }
        
        if (first_unacked != next_seqnum) {
          timer_seq = first_unacked;
          safe_start_timer(A, RTT);
        }
      }
    }
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
  timer_seq = 0;
  timer_running = false;
  
  /* Initialize send buffer and status */
  for (i = 0; i < WINDOWSIZE; i++) {
    send_status[i] = UNUSED;
    retransmission_count[i] = 0;  /* Initialize retransmission counters */
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
    int window_end;

    /* For the receiver, we also need to acknowledge packets right before the window */ 
    if (seqnum == ((recv_base - 1 + SEQSPACE) % SEQSPACE))
        return false;

    window_end = (recv_base + WINDOWSIZE - 1) % SEQSPACE;
    
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
  int index;
  
  /* Count this packet as correctly received if it's not corrupted */
  if (!IsCorrupted(packet)) {
    packets_received++;
  }  
  
  /* Check if packet is corrupted */
  if (IsCorrupted(packet)) {
    if (TRACE > 1)
      printf("----B: packet corrupted, resend ACK!\n");
    
    /* First check if the packet is corrupted */
    if (last_ack_sent != -1) {
      sendpkt.acknum = last_ack_sent;
    } else {
      /* If no packet has been correctly received yet, just use recv_base-1 */
      sendpkt.acknum = (recv_base - 1 + SEQSPACE) % SEQSPACE;
    }
  }
  /* Then check if it's within the receive window */
  else if (!in_recv_window(packet.seqnum)) {
    /* This is a packet outside the receive window */
    /* If it's the packet just before the window, it's a duplicate we've already processed */
    if (packet.seqnum == ((recv_base - 1 + SEQSPACE) % SEQSPACE)) {
      if (TRACE > 1)
        printf("----B: packet %d is correctly received, send ACK!\n", packet.seqnum);
      
      /* Send ACK for this packet since it's a duplicate of the last packet we delivered */
      sendpkt.acknum = packet.seqnum;
      last_ack_sent = packet.seqnum;
    } else {
       /* For any other packet outside the window, we need to send an ACK for the last packet */
      if (TRACE > 1)
        printf("----B: packet %d is correctly received, send ACK!\n", packet.seqnum);
      
      if (last_ack_sent != -1) {
        sendpkt.acknum = last_ack_sent;
      } else {
        sendpkt.acknum = (recv_base - 1 + SEQSPACE) % SEQSPACE;
      }
    }
  }
  else {
    if (TRACE > 1) 
      printf("----B: packet %d is correctly received, send ACK!\n", packet.seqnum);

    index = recv_seq_to_index(packet.seqnum);

    /* If we haven't received this packet before */
    if (!recv_status[index]) {
      /* Store packet in buffer */
      recv_buffer[index] = packet;
      recv_status[index] = true;
    
      /* If this is the packet we're waiting for, deliver it and any consecutive buffered packets */
      if (packet.seqnum == recv_base) {
    
        while (recv_status[recv_seq_to_index(recv_base)]) {
          /* Deliver packet to layer 5 */
          index = recv_seq_to_index(recv_base);
          tolayer5(B, recv_buffer[index].payload);
          
          /* Mark buffer slot as empty */
          recv_status[index] = false;
          
          /* Advance receive window */
          recv_base = (recv_base + 1) % SEQSPACE;
        }
      }
    } 
        
    /* Send ACK for this packet */
    sendpkt.acknum = packet.seqnum;
    last_ack_sent = packet.seqnum;
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
  last_ack_sent = -1;  /* Initialize to indicate no ACK sent yet */

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

