#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "lib.h"

#define HOST "127.0.0.1"
#define PORT 10001

int main(int argc, char** argv) {
    msg t;
    msg *y;
    int sequence = 1, retry = 0;
    unsigned short crc;
    mini_k1 pack1;
    mini_k2 pack2;
    s_data data;
    init(HOST, PORT);

    //Primire pachet tip S pentru initializare transmisie
    while(retry != 3){
        y = receive_message_timeout(5000);
        if(y == NULL)
            retry++;
        else{
            //Verificare daca pachetul e corupt sau nu si trimitere ACK/NAK
            retry = 0;
            memcpy(&pack1, y->payload, y->len - 3);
            memcpy(&pack2, y->payload + y->len - 3, sizeof(mini_k2));
            crc = crc16_ccitt(y->payload, y->len - 3);
            if(crc == pack2.CHECK && pack1.SEQ == sequence - 1 && pack1.TYPE == 'S'){
                pack1.TYPE = 'Y';
                pack1.SEQ = sequence; 
                t.len = 18;
                memcpy(&data, pack1.DATA, sizeof(s_data));
                memcpy(t.payload, &pack1, t.len - 3);
                memcpy(t.payload + t.len - 3, &pack2, sizeof(mini_k2));
                send_message(&t);
                break;
            }
            else{
                pack1.TYPE = 'N';
                pack1.SEQ = sequence;
                t.len = 18;
                memcpy(t.payload, &pack1, t.len - 3);
                memcpy(t.payload + t.len - 3, &pack2, sizeof(mini_k2));
                send_message(&t); 
            }
        }
    }
    
    //Daca se ia timeout de 3 ori se incheie executia
    if(y == NULL){
        printf("[Receiver] Transmition ended after 3 timeouts.\n");
        return -1;
    }

    for(int k = 0; k < max_len; k++) 
        pack1.DATA[k] = 0;

    //Primirea fisierelor
    int end_transmition = 1;
	while(end_transmition == 1){
        char filename[100] = "recv_";

        sequence += 2; sequence %= 64;
        retry = 0;

        //Primire pachet de tip F si raspundere cu ACK/NAK(daca e corupt)
        //De asemenea tot aici fac verificarea si pentru finalul transmisiei
        while(retry != 3){
            y = receive_message_timeout(5000);
            if(y == NULL){
                retry++;
            }
            else{
                retry = 0;
                memcpy(&pack1, y->payload, y->len - 3);
                memcpy(&pack2, y->payload + y->len - 3, sizeof(mini_k2));
                crc = crc16_ccitt(y->payload, y->len - 3);
                //Verificare pachet si trimitere ACK/NAK
                if(crc == pack2.CHECK){ 
                    if(pack1.TYPE == 'F'){
                        strcat(filename, pack1.DATA);
                    }
                    else 
                        if(pack1.TYPE == 'B')
                            end_transmition = 0;
                    t.len = 7;
                    pack1.TYPE = 'Y';
                    pack1.SEQ = sequence;
                    for(int k = 0; k < max_len; k++) pack1.DATA[k] = 0;
                    strcpy(pack1.DATA, "");
                    pack1.LEN = t.len - 2;
                    memcpy(t.payload, &pack1, t.len - 3);
                    memcpy(t.payload + t.len - 3, &pack2, sizeof(mini_k2));
                    if(end_transmition == 0) 
                        goto end;
                    send_message(&t);
                    break;
                }
                else{
                    t.len = 7;
                    pack1.TYPE = 'N';
                    pack1.SEQ = sequence;
                    for(int k = 0; k < max_len; k++) pack1.DATA[k] = 0;
                    strcpy(pack1.DATA, "");
                    pack1.LEN = t.len - 2;
                    memcpy(t.payload, &pack1, t.len - 3);
                    memcpy(t.payload + t.len - 3, &pack2, sizeof(mini_k2));
                    send_message(&t); 
                }
            }
        }

        if(y == NULL){
            printf("[Receiver] Transmition ended after 3 timeouts.\n");
            return -1;
        }

        //Creare fisier dupa primire file header
        creat(filename, 0777);
        int fd = open(filename, O_WRONLY);

        //Primesc pachete de tip D pana la finalul fisierului(Z) si 
        //trimit inapoi ACK/NAK daca pachetul a fost corupt sau nu
        int end_file = 1;
        while(end_file){
            sequence += 2; sequence %= 64;
            retry = 0;
 
            //Primire pachet
            while(retry != 3){
                y = receive_message_timeout(5000);
                if(y == NULL){
                    retry++; 
                }
                else{
                    retry = 0;
                    memcpy(&pack1, y->payload, y->len - 3);
                    memcpy(&pack2, y->payload + y->len - 3, sizeof(mini_k2));
                    crc = crc16_ccitt(y->payload, y->len - 3);
                    //Trimitere ACK/NAK
                    if(crc == pack2.CHECK){ 
                        if(pack1.TYPE == 'D')
                            write(fd, pack1.DATA, y->len - 7);
                        else 
                            if(pack1.TYPE == 'Z'){
                                end_file = 0;
                                close(fd);
                            }
                        t.len = 7;
                        pack1.TYPE = 'Y';
                        pack1.SEQ = sequence;
                        for(int k = 0; k < max_len; k++) pack1.DATA[k] = 0;
                        strcpy(pack1.DATA, "");
                        pack1.LEN = t.len - 2;
                        memcpy(t.payload, &pack1, t.len - 3);
                        memcpy(t.payload + t.len - 3, &pack2, sizeof(mini_k2));
                        send_message(&t);
                        break;
                    }
                    else{
                        t.len = 7;
                        pack1.TYPE = 'N';
                        pack1.SEQ = sequence;
                        for(int k = 0; k < max_len; k++) pack1.DATA[k] = 0;
                        strcpy(pack1.DATA, "");
                        pack1.LEN = t.len - 2;
                        memcpy(t.payload, &pack1, t.len - 3);
                        memcpy(t.payload + t.len - 3, &pack2, sizeof(mini_k2));
                        send_message(&t); 
                    }
                }
            }

            if(y == NULL){
                printf("[Receiver] Transmition ended after 3 timeouts.\n");
                return -1;
            }
        }  
    }

    end:send_message(&t);

	return 0;
}
