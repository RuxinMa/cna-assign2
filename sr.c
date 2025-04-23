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

/* generic procedure to compute the checksum of a packet.  Used by both sender and receiver  
   the simulator will overwrite part of your packet with 'z's.  It will not overwrite your 
   original checksum.  This procedure must generate a different checksum to the original if
   the packet is corrupted.
*/

/* Array to track retransmission counts for each packet in the window */ 
static int retransmission_count[WINDOWSIZE] = {0};
static int last_ack_sent = -1;  /* Last ACK number that was sent by receiver */

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
    int window_end = (send_base + WINDOWSIZE - 1) % SEQSPACE;
    
    if (send_base <= window_end) {
        /* Normal case: window doesn't wrap around */
        return (seqnum >= send_base && seqnum <= window_end);
    } else {
        /* window wraps around */
        return (seqnum >= send_base || seqnum <= window_end);
    }
}

/* called from layer 5 (application layer), passed the message to be sent to other side */
void A_output(struct msg message)
{
  struct pkt sendpkt;
  int i;
  int index;

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
      starttimer(A, RTT);
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
  int index; /* Move declaration to the beginning */

  /* if received ACK is not corrupted */ 
  if (!IsCorrupted(packet)) {
    if (TRACE > 0)
      printf("----A: uncorrupted ACK %d is received\n",packet.acknum);
    /* total_ACKs_received++; */

    /* Check if the ACK is for a packet in our send window or the last acknowledged packet */
    if (in_send_window(packet.acknum) || 
        packet.acknum == ((send_base - 1 + SEQSPACE) % SEQSPACE)) {

      index = seq_to_index(packet.acknum);
      
      /* Check if this packet hasn't been ACKed yet */
      if (packet.acknum == ((send_base - 1 + SEQSPACE) % SEQSPACE) || 
          (in_send_window(packet.acknum) && send_status[index] == SENT)) {
        
        /* Only mark as new ACK if it's for a previously unacked packet in our window */
        if (in_send_window(packet.acknum) && send_status[index] == SENT) {
          /* Mark packet as acknowledged */
          send_status[index] = ACKED;
          retransmission_count[index] = 0;  /* Reset retransmission counter */
          
          if (TRACE > 0)
            printf("----A: ACK %d is not a duplicate\n",packet.acknum);
          new_ACKs++;
        } else {
          if (TRACE > 0)
            printf("----A: ACK %d is a duplicate (for packet before window)\n", packet.acknum);
        }
        
        if (packet.acknum == send_base) {
          stoptimer(A);
        
          /* Slide window over all consecutively ACKed packets */
          while (send_base != next_seqnum && 
                 send_status[seq_to_index(send_base)] == ACKED) {
            /* Mark slot as unused */
            send_status[seq_to_index(send_base)] = UNUSED;
            /* Slide window by one */
            send_base = (send_base + 1) % SEQSPACE;
          }

          /* If we need to restart the timer and there are unacked packets */
          if (send_base != next_seqnum) {
            starttimer(A, RTT);
          }
        }
      }
      else {
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
  if (TRACE > 0)
    printf("----A: time out,resend packets!\n");
  
  /* Only process if there are unacked packets */
  if (send_base != next_seqnum) {
    int index = seq_to_index(send_base);

    /* Check if we've reached the maximum retransmission limit */
    if (retransmission_count[index] < MAX_RETRANSMISSIONS) {
      if (TRACE > 0)
        printf("---A: resending packet %d\n", send_base);
      
      tolayer3(A, send_buffer[index]); 
      packets_resent++;
      retransmission_count[index]++;
      
      starttimer(A, RTT);
    } else {
      /* Max retransmissions reached, assume packet was received */
      if (TRACE > 0)
        printf("---A: packet %d reached max retransmissions, marking as ACKED\n", send_base);
      
      /* Mark as ACKed to move window forward */
      send_status[index] = ACKED;

      /* Slide window */
      while (send_base != next_seqnum && 
             send_status[seq_to_index(send_base)] == ACKED) {
        /* Mark slot as unused */
        send_status[seq_to_index(send_base)] = UNUSED;
        /* Reset retransmission counter */
        retransmission_count[seq_to_index(send_base)] = 0;
        /* Slide window by one */
        send_base = (send_base + 1) % SEQSPACE;
      }
    
      /* Restart timer if more packets to send */
      if (send_base != next_seqnum) {
        starttimer(A, RTT);
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
  int index;  
  
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
    if (TRACE > 1)
      printf("----B: packet not in receive window, resend ACK!\n");
    
    /* Handle the case of duplicate packets before the window */
    if (((recv_base - packet.seqnum) % SEQSPACE) <= WINDOWSIZE){
      /* This is likely a retransmission of an already received packet */
      sendpkt.acknum = packet.seqnum;
      last_ack_sent = packet.seqnum;
    } else {
      /* Otherwise use the last sent ACK */
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
          packets_received++;
          
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

