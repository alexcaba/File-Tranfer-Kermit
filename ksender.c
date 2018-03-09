#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "lib.h"

#define HOST "127.0.0.1"
#define PORT 10000

int main(int argc, char** argv) {
    msg t;
    msg *y;
    char buffer[max_len];
    int sequence = 0, retry = 0, read_size;
    unsigned short crc;
    mini_k1 pack1;
    mini_k2 pack2;
    s_data data;
    init(HOST, PORT);

    //Scriere pachet de tip S pentru initializare transmisie
    pack1.SOH = 0x01;
    pack1.LEN = 18;
    pack1.SEQ = sequence;
    pack1.TYPE = 'S';

    data.MAXL = max_len;
    data.TIME = time;
    data.NPAD = 0x00;
    data.PADC = 0x00;
    data.EOL = 0x0D;
    data.QCTL = 0x00;
    data.QBIN = 0x00;
    data.CHKT = 0x00;
    data.REPT = 0x00;
    data.CAPA = 0x00;
    data.R = 0x00;
    t.len = 18;

    //Scriere pachet S in payload
    for(int k = 0; k < max_len; k++) 
        pack1.DATA[k] = 0;
    memcpy(pack1.DATA, &data, sizeof(s_data));
    memcpy(t.payload, &pack1, t.len - 3);

    crc = crc16_ccitt(t.payload, t.len - 3);
    pack2.CHECK = crc;
    pack2.MARK = 0x0D;
    memcpy(t.payload + t.len - 3, &pack2, sizeof(mini_k2));

    //Trimitere pachet S pana la confirmare cu ACK
    while(retry != 3){
        send_message(&t);
        y = receive_message_timeout(5000);
        if(y == NULL){
            retry++;
        }
        else{
            retry = 0;
            memcpy(&pack1, y->payload, y->len - 3);
            if(pack1.TYPE == 'Y')
                break;
        }
    }

    //Daca se ia timeout de 3 ori se incheie executia
    if(y == NULL){
        printf("[Sender] Transmition ended after 3 timeouts.\n");
        return -1;
    }

    //Trimiterea fisierelor
    for(int i = 1; i < argc ; i++){
        //Scriere pachet de tip F
        sequence += 2; sequence %= 64;
        int fd = open(argv[i], O_RDONLY);
        t.len = strlen(argv[i]) + 7;
        pack1.LEN = t.len - 2;
        pack1.SEQ = sequence;
        pack1.TYPE = 'F';

        for(int k = 0; k < max_len; k++) 
            pack1.DATA[k] = 0;
        strcpy(pack1.DATA, argv[i]);

        memcpy(t.payload, &pack1, t.len - 3);
        crc = crc16_ccitt(t.payload, t.len - 3);
        pack2.CHECK = crc;
        memcpy(t.payload + t.len - 3, &pack2, sizeof(mini_k2));

        //Trimitere pachet F pana la confirmare cu ACK
        retry = 0;
        while(retry != 3){
            send_message(&t); 
            y = receive_message_timeout(5000);
            if(y == NULL){
                retry++;
            }
            else{
                retry = 0;
                memcpy(&pack1, y->payload, y->len - 3);
                if(pack1.TYPE == 'Y') 
                    break;
            }
        }

        if(y == NULL){
            printf("[Sender] Transmition ended after 3 timeouts on file: %s.\n", argv[i]);
            return -1;
        }

        //Trimitere pachete din fisier pana la sfarsitul acestuia
        while( (read_size = read(fd, buffer, max_len - 1)) > 0){
            sequence += 2; sequence %= 64;
            t.len = read_size + 7;
            for(int k = 0; k < max_len; k++) 
                pack1.DATA[k] = 0;
            memcpy(pack1.DATA, &buffer, read_size);
            pack1.LEN = t.len - 2;
            pack1.SEQ = sequence;
            pack1.TYPE = 'D';
            memcpy(t.payload, &pack1, t.len - 3);
            pack2.CHECK = crc16_ccitt(t.payload, t.len - 3);
            memcpy(t.payload + t.len - 3, &pack2, sizeof(mini_k2));

            //Trimitere pachet D pana la confirmare cu ACK
            retry = 0;
            while(retry != 3){
                send_message(&t); 
                y = receive_message_timeout(5000);
                if(y == NULL){
                    retry++;
                }
                else{
                    retry = 0;
                    memcpy(&pack1, y->payload, y->len - 3);
                    if(pack1.TYPE == 'Y') 
                        break;
                }
            }

            if(y == NULL){
                printf("[Sender] Transmition ended after 3 timeouts on file: %s.\n", argv[i]);
                return -1;
            }
        }

        //Trimitere pachet pentru final de fisier
        sequence += 2; sequence %= 64;
        t.len = 7;
        for(int k = 0; k < max_len; k++) pack1.DATA[k] = 0;
        strcpy(pack1.DATA, "");
        pack1.LEN = t.len - 2;
        pack1.SEQ = sequence;
        pack1.TYPE = 'Z';
        memcpy(t.payload, &pack1, t.len - 3);
        pack2.CHECK = crc16_ccitt(t.payload, t.len - 3);
        memcpy(t.payload + t.len - 3, &pack2, sizeof(mini_k2));

        //Trimitere pachet Z pana la confirmare cu ACK
        retry = 0;
        while(retry != 3){
            send_message(&t); 
            y = receive_message_timeout(5000);
            if(y == NULL){
                retry++;
            }
            else{
                retry = 0;
                memcpy(&pack1, y->payload, y->len - 3);
                if(pack1.TYPE == 'Y') 
                    break;
            }
        }

        if(y == NULL){
            printf("[Sender] Transmition ended after 3 timeouts on file: %s.\n", argv[i]);
            return -1;
        }
    }

    //Daca s-au trimis toate fisierele se incheie transmisia si trimit ultimul pachet
    sequence += 2; sequence %= 64;
    t.len = 7;
    pack1.LEN = t.len - 2;
    pack1.SEQ = sequence;
    pack1.TYPE = 'B';
    for(int k = 0; k < max_len; k++) pack1.DATA[k] = 0;
    strcpy(pack1.DATA, "");
    memcpy(t.payload, &pack1, t.len - 3);
    pack2.CHECK = crc16_ccitt(t.payload, t.len - 3);
    memcpy(t.payload + t.len - 3, &pack2, sizeof(mini_k2));

    //Trimitere pachet B pana la confirmare cu ACK
    retry = 0;
    while(retry != 3){
        send_message(&t); 
        y = receive_message_timeout(5000);
        if(y == NULL){
            retry++;
        }
        else{
            retry = 0;
            memcpy(&pack1, y->payload, y->len - 3);
            if(pack1.TYPE == 'Y') 
                break;
            }
        }
        
    if(y == NULL){
        printf("[Sender] Transmition ended after 3 timeouts.\n");
        return -1;
    }

    return 0;
}
