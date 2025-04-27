# Git log 

### commit b4edc84d6ef9eb15f5879f2c13dbaed9f724aab9
Author: RuxinMa <alisonma0610@gmail.com>
Date:   Sun Apr 27 21:47:49 2025 +0930

    fix ACK processing logic, modify timer management to only time one pkt and add checks to prevent improper timer access

commit 7552939fe467c3fd42e929a03eaa2bba18e02f8f
Author: RuxinMa <alisonma0610@gmail.com>
Date:   Sun Apr 27 18:03:02 2025 +0930

    modify the MAX_RETRANSMIT val, update git log messages
    
### commit c50dab348ab0d3684c4a9f544a4b38b8740314e9 
Author: RuxinMa <alisonma0610@gmail.com>
Date:   Sun Apr 27 17:18:21 2025 +0930

    resolve infinite loop and improve, add MAX_RETRANSMIT and function for window sliding

### commit 6fb40e9f2a077da457b9b62a93cbb86a16f58894
Author: RuxinMa <alisonma0610@gmail.com>
Date:   Fri Apr 25 16:03:38 2025 +0930

    rollback to commit 1fa5c8539a3c857aafcd16ea9e8cee1180fecd1a,  reverting to previous stable implementation, resolve mixed declarations

### commit 0f48b5cfecde625498acfc65542f453c9045b77d
Author: RuxinMa <alisonma0610@gmail.com>
Date:   Thu Apr 24 01:41:49 2025 +0930

    update git log messages

### commit 3dc636771b4588dfa6aa278b729919ec14fb4beb
Author: RuxinMa <alisonma0610@gmail.com>
Date:   Thu Apr 24 01:38:15 2025 +0930

    fix bugs and remove the logic that checks for a maximum number of retransmissions

### commit 38154a87b7fe2eb489b13b75458a8bad3e203aa1
Author: RuxinMa <alisonma0610@gmail.com>
Date:   Wed Apr 23 18:58:28 2025 +0930

    fix SR ACK counting  errors and retransmission handling, write updated test cases to verify output from autograder

### commit ae5aefcc4c18f9029afa816f00f42499a73f4ef9
Author: RuxinMa <alisonma0610@gmail.com>
Date:   Wed Apr 23 15:57:34 2025 +0930

    fix retransmission logic to resend all unacknowledged packets in the window, to prevent infinite loops

###  commit 9e54367c0e95a31d453a21738df6ef62af22117c
Author: RuxinMa <alisonma0610@gmail.com>
Date:   Tue Apr 22 21:35:10 2025 +0930

    add retransmissions to solve the error of timer out when auto testing

###  commit a079cac2623f973172b8379abba76d0128f15af6
Author: RuxinMa <alisonma0610@gmail.com>
Date:   Tue Apr 22 21:01:04 2025 +0930

    modify readme, add git log messages

### commit a744b019c9c3d7293b80abe92fa7834e0f7173b6
Author: RuxinMa <alisonma0610@gmail.com>
Date:   Tue Apr 22 20:47:41 2025 +0930

    fix trace error, delete the logic of retransmission, modify the timer mechanism

### commit 71c62fbc6119f3055247bc6828664e3bca3288c6
Author: RuxinMa <alisonma0610@gmail.com>
Date:   Tue Apr 22 19:57:49 2025 +0930

    fix the timer mechanism bugs shown in full test, ensure timer is always started when sending packets

### commit 251cc0cb5cdfc3163f75dfe05b5d88c7111800b4
Author: RuxinMa <alisonma0610@gmail.com>
Date:   Tue Apr 22 18:21:18 2025 +0930

    fix the autograder trace error, modify sr prints when TRACE > 0 to only print when TRACE == 1

### commit bb6115bcae342176158b3cd3c93a9f614c0837d5
Author: RuxinMa <alisonma0610@gmail.com>
Date:   Tue Apr 22 18:07:44 2025 +0930

    impletment advanced testing on loss and corruption, high traffic load and verify long-term stability

### commit 23b0aa46378ea62f20e4fe27a588fc52b4a7b2aa
Author: RuxinMa <alisonma0610@gmail.com>
Date:   Tue Apr 22 17:12:47 2025 +0930

    implement testing on data packet corruption, combined loss and corruption, ACK loss and corruption

### commit ef0e4d5cb53cecac28548e103fd72d4f096db34d
Author: RuxinMa <alisonma0610@gmail.com>
Date:   Mon Apr 21 22:50:45 2025 +0930

    start testing on basic case, packet loss and corruption to verify
    he logic and behaviour of the program

### commit d649e1f18f65184a21784956fabe28880b258b39
Author: RuxinMa <alisonma0610@gmail.com>
Date:   Mon Apr 21 22:15:35 2025 +0930

    fix bug: use *sim_time to reference the emulator's time variable without causing a naming conflict

### commit 2be9d040721c1183b0bf7f497d564ae15439eda0
Author: RuxinMa <alisonma0610@gmail.com>
Date:   Mon Apr 21 21:54:18 2025 +0930

    fix syntax bugs and re compile the program

### commit 5edcdec4cf0adf2af4c3634b0af9236f0006197c
Author: RuxinMa <alisonma0610@gmail.com>
Date:   Mon Apr 21 21:42:48 2025 +0930

    try autograder testing, convert all comments (//) to comments (/* */) to fix bugs

### commit a54ef240d970e67a1d2bd8411e1e3b99305b48fb
Author: RuxinMa <alisonma0610@gmail.com>
Date:   Mon Apr 21 21:36:36 2025 +0930

    add MAX_RETRANSMISSIONS logic to prevent infinite loops, and test different case of SR

### commit 93ee5f51ece29f1947a3a85c802e9601b9faa812
Author: RuxinMa <alisonma0610@gmail.com>
Date:   Mon Apr 21 17:12:36 2025 +0930

    modify A_timerinterrupt(), B_input() and A_input(), to resolve the infinite loop issue, and re-test

### commit 72b869b1c95d29e666eb07a6a67092cf1c9a0af7
Author: RuxinMa <alisonma0610@gmail.com>
Date:   Mon Apr 21 16:03:55 2025 +0930

    fix the bug of in_send_window() and in_recv_window(), restart test

### commit dfab3482649319be68a508da01c859dec2c2481c
Author: RuxinMa <alisonma0610@gmail.com>
Date:   Mon Apr 21 15:47:44 2025 +0930

    start testing and debugging, write a simple tset script to save test results

### commit 2ababd3b94bac95dc41f99b4427181c97fda7cf7
Author: RuxinMa <alisonma0610@gmail.com>
Date:   Sun Apr 20 21:52:33 2025 +0930

    fix bug: update emulator.c, fix the braces in the A_input, define B_nextseqnum in the receiver; implement basic test to verify

### commit 61994614892140fc377a0d4fa94961a9aedb4f46
Author: RuxinMa <alisonma0610@gmail.com>
Date:   Sun Apr 20 21:03:58 2025 +0930

    add helper functions for B_input function implementation

### commit fb5082910e3e4a4bc16f18197a37b6e5e3485aae
Author: RuxinMa <alisonma0610@gmail.com>
Date:   Sun Apr 20 18:05:35 2025 +0930

    modify receiver B data structure and B_init to initialize new stucture, update the B_input function for SR protocol

### commit 170dae5e9683a0ca1deb6d8625df60f7b80b5cf0
Author: RuxinMa <alisonma0610@gmail.com>
Date:   Sun Apr 20 17:28:40 2025 +0930

    modify A_timerinterrup to handle timeout, only resend packets that have timed out but haven't been acknowledged

### commit 245ecaa23951e2999d35d0db0b7a389303f5311d
Author: RuxinMa <alisonma0610@gmail.com>
Date:   Sun Apr 20 16:31:09 2025 +0930

    fix A_input() function to be updated for SR individual packet ACK

### commit 995947a236e0e81c5666c90efda3e9cf7351b071
Author: RuxinMa <alisonma0610@gmail.com>
Date:   Sun Apr 20 02:04:50 2025 +0930

    add a new helper function has_running_timer() for A_output() implementation

### commit 16103ecb1d1bf8dbc956840295eed09a316f3a9d
Author: RuxinMa <alisonma0610@gmail.com>
Date:   Sun Apr 20 01:57:03 2025 +0930

    modify A_output() function logic for SR, fix SEQSPACE for SR

commit d78420c511078898da80f9df23b839b076d2acdb
Author: RuxinMa <alisonma0610@gmail.com>
Date:   Sat Apr 19 23:38:30 2025 +0930

    modify A_init() to initialize SR-specific variables

### commit 8a190d09e27ee979474e475a01d81bc433adc29e
Author: RuxinMa <alisonma0610@gmail.com>
Date:   Sat Apr 19 23:26:00 2025 +0930

    define new data structures for packet status tracking of the selective repeat protocol

### commit 5fa99a5ab3c2b5e8fe908cdf997c33abcf220e6f
Author: RuxinMa <alisonma0610@gmail.com>
Date:   Sat Apr 19 23:25:07 2025 +0930

    define new data structures for packet status tracking of the selective repeat protocol

### commit 7465690367c6526a33da0958c3ddd343428f6854
Author: RuxinMa <alisonma0610@gmail.com>
Date:   Sat Apr 19 22:46:01 2025 +0930

    copy the gbn.c file to a file called sr.c and copy gbn.h to sr.h

### commit 38e0f63c4a7dbea27b75678273b995eea1d1b05b
Author: RuxinMa <alisonma0610@gmail.com>
Date:   Sat Apr 19 22:44:06 2025 +0930

    build the go-back-N  program, test the simulated network with different parameters

### commit 53f1f37f4c192e4840313d5fcf890d6c0cbb1f2b
Author: RuxinMa <alisonma0610@gmail.com>
Date:   Sat Apr 19 22:08:17 2025 +0930

    understand the background and the aim of this assginment, download the code

### commit 15d2988e752e2f8ef0eb0e6d344dba29f3a3b214
Author: RuxinMa <alisonma0610@gmail.com>
Date:   Sat Apr 19 22:05:07 2025 +0930

    first commit