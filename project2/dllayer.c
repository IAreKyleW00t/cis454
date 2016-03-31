#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netdb.h>
#include "crc.h"
#include "dllayer.h"
#include "dlsendto.h"

static int sk;
static struct sockaddr_in server_remote;
static unsigned char seq = 0;
static unsigned int count = 1;

bool dlinit_send(char *hostname, unsigned short port) {
    /* Create a UDP socket */
    sk = socket(AF_INET, SOCK_DGRAM, 0);
    if (!sk) {
        fprintf(stderr, "socket: Failed to open socket: %s\n", strerror(errno));
        return false;
    }
    
    server_remote.sin_family = AF_INET;
  
    /* Save hostname */
    struct hostent *hp = gethostbyname(hostname);
    if (!hp) {
        fprintf(stderr, "gethostbyname: Failed to open socket: %s\n", hstrerror(h_errno));
        return false;
    }
    bcopy(hp->h_addr, &server_remote.sin_addr.s_addr, hp->h_length);
    
    /* Save port */
    server_remote.sin_port = ntohs(port);
    return true;
}

bool dlinit_recv(unsigned short port) {
    struct sockaddr_in local;

    /* Create a UDP socket */
    sk = socket(AF_INET, SOCK_DGRAM, 0);
    if (!sk) {
        fprintf(stderr, "socket: Failed to open socket: %s\n", strerror(errno));
        return false;
    }

    /* Set socket attributes */
    local.sin_family = AF_INET;
    local.sin_addr.s_addr = INADDR_ANY;
    local.sin_port = htons(port);
    
    /* Attempt to bind port */
    if (bind(sk, (struct sockaddr *)&local, sizeof(local)) < 0) {
        fprintf(stderr, "bind: Failed to bind socket: %s\n", hstrerror(h_errno));
        return false;
    }
    
    return true;
}

ssize_t dlsend(unsigned char data[], size_t len, bool eof) {
    /* Return -1 if dl_init has not been called successfully */
    if (!sk) {
        fprintf(stderr, "dlsend: Failed to send data: Cannot write to NULL socket\n");
        return -1;
    }
    
    /* Create our output frame and keep track of the
        total bytes we send and the size of our frame. */
    unsigned char frame[128], ack[3];
    ssize_t total_bytes_sent = len, size;
    do {
        /* Determine if we have a full frame or a partial frame.
            This can be a maximum of 125 bytes (128 total including
            our header and CRC). */
        size = FRAME_SIZE(len);
        memset(frame, 0x00, sizeof(frame)); //Zero out the frame (Safety precaution.)
        
        /* Set our header values */
        if (eof && len <= 125) SET_BIT(frame[0], 5);
        
        /* Set our SEQ to 1 if needed. */
        if (seq == 1) SET_BIT(frame[0], 3);
        _DEBUG("Frame #%d (seq %d)\n", count, seq);
        
        /* Save our input data into the frame based on the
            size we calculated before. We beginning writing
            at (frame + 1) to skip the header byte. */
        memcpy((frame + 1), data, size);
        
        /* Calculate the CRC for our frame and save it into
            the last 2 bytes of our frame. */
        unsigned short c = crc(frame, (size + 3));
        frame[size + 1] = (c >> 8) & 0xFF; //MSB
        frame[size + 2] = (c >> 0) & 0xFF; //LSB
        
        int retries = 0;
        do {
            retries++; //Increment number of retries
            
            /* Send our frame to the server. */
            dlsendto(sk, frame, (size + 3), 0, (struct sockaddr *)&server_remote, sizeof(server_remote), 1);
            _DEBUG("  > Frame sent\n");
            
            /* Create and set up variables for select()
                later on. */
            fd_set rfds;
            struct timeval tv;
            int ret;
            FD_ZERO(&rfds);
            FD_SET(sk, &rfds);
            tv.tv_sec = 1;
            tv.tv_usec = 0;
            
            /* Wait for an ACK back from the server. */
            _DEBUG("  > Waiting\n");
            ret = select((sk + 1), &rfds, NULL, NULL, &tv);
            if (ret == -1) { //Error
                fprintf(stderr, "select: Failed to wait on socket: %s\n", strerror(errno));
                return total_bytes_sent;
            } else if (ret) { //Got response
                read(sk, ack, sizeof(ack));
            } else { //Timeout
                _DEBUG("  > Timeout\n");
                continue;
            }
            
            /* Check if ACK is valid (no errors). */
            unsigned short c_ack = crc(ack, sizeof(ack));
            if (c_ack != 0) {
                _DEBUG("  > Ignore ACK\n");
                continue;
            }
            
            /* Check if ACK is actually an ACK. */ 
            if (!CHECK_BIT(ack[0], 1)) {
                _DEBUG("  > Invalid ACK\n");
                continue;
            }
            
            /* Check if ACK matches the SEQ we expected. */
            if (GET_BIT(ack[0], 3) != seq) {
                _DEBUG("  > Duplicate ACK\n");
                continue;
            }
        
            _DEBUG("  > Accept ACK\n");
            
            /* Break out of loop because we got a valid response. */
            break;
        } while (retries < MAX_RETRY);
        
        /* Check if we hit the MAX_RETRY limit. */
        if (retries == MAX_RETRY) {
            fprintf(stderr, "dlsend: Failed to send data: Retry limit reached\n");
            return -1;
        }
        
        /* Move our pointers so we are ready for the
            next incoming frame and flip SEQ number. */
        data += size;
        len -= size;
        seq = !seq;
        count++;
    } while (len > 0);
    
    /* Return the total number of bytes that we sent. */
    return total_bytes_sent;
}

ssize_t dlrecv(unsigned char data[], size_t len, bool *eof){
    /* Return -1 if dl_init has not been called successfully */
    if (!sk) {
        fprintf(stderr, "dlsend: Failed to send data: Cannot read from NULL socket\n");
        return -1;
    }

    /* EOF is false by default */
    *eof = false;
    
    /* Create our input frame and keep track of the
        total bytes we receive and the size of our frame. */
    unsigned char frame[128], ack[3];
    ssize_t total_bytes_recv = 0, size;
    
    /* Create our remote socket in order to respond
        to the client who sent the frame. */
    struct sockaddr_in client_remote;
    socklen_t clen = sizeof(client_remote);    
    
    /* Only continue to recv data if we know we
        can fit it into our buffer. */
    while (len >= 125) {
        _DEBUG("Frame #%d (seq %d)\n", count, seq);
        _DEBUG("  > Waiting\n");
        /* Wait for a frame from the client and save the
            size of the frame we get. (Max of 128 bytes) */
        size = recvfrom(sk, frame, sizeof(frame), 0, (struct sockaddr *)&client_remote, &clen);
        
        /* Calculate the CRC for our frame and check if it is valid.
            If the message is invalid (error occurred) then
            we ignore the frame entirely and do not ACK. */
        unsigned short c = crc(frame, size);
        if (c != 0) {
            _DEBUG("  > Ignore frame\n");
            continue;
        }
        
        /* Zero out our ACK to give it guaranteed default values. */
        memset(ack, 0x00, sizeof(ack));
        SET_BIT(ack[0], 1);
        
        /* Set our SEQ to 1 if needed. */
        if (seq == 1) SET_BIT(ack[0], 3);
        
        unsigned short c_ack = crc(ack, sizeof(ack));
        ack[1] = (c_ack >> 8) & 0xFF; //MSB
        ack[2] = (c_ack >> 0) & 0xFF; //LSB
        
        /* Check if frame is a duplicate. */
        if (GET_BIT(frame[0], 3) != seq) {
            _DEBUG("  > Duplicate frame\n");

            /* Send an ACK back to the client. */
            dlsendto(sk, ack, sizeof(ack), 0, (struct sockaddr *)&client_remote, sizeof(client_remote), 1);
            _DEBUG("  > Ack sent\n");
            continue;
        }
        
        /* Copy the data within our frame to our output buffer.
            We read from (frame + 1) to (size - 3) in order to
            skip the header and CRC. */
        memcpy(data, (frame + 1), (size - 3));
        
        /* Check if we reached EOF. */
        if (CHECK_BIT(frame[0], 5)) *eof = true;
        
        /* Send an ACK back to the client. */
        dlsendto(sk, ack, sizeof(ack), 0, (struct sockaddr *)&client_remote, sizeof(client_remote), 1);
        _DEBUG("  > Ack sent\n");
        
        /* Move our pointers so we are ready for the
            next incoming frame. */
        data += (size - 3);
        len -= (size - 3);
        total_bytes_recv += (size - 3);
        seq = !seq;
        count++;
        
        /* Check if we reached EOF. If true, we can
            terminate the loop at this point. */
        if (eof) break;
    }
    
    /* Return the total number of bytes that we received. */
    return total_bytes_recv;
}
